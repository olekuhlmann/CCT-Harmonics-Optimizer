#include "linear_optimizer.h"

using CCTools::Logger;

LinearOptimizer::LinearOptimizer(std::string optimized_value_label, std::string harmonic_drive_prefix) : AbstractOptimizer(false), optimized_value_label_(optimized_value_label)
{
    CCTools::ModelHandler model_handler = initModel();
    double max_value = getMaxHarmonicValue();
    setup(model_handler, max_value, harmonic_drive_prefix);
}

LinearOptimizer::LinearOptimizer(std::string optimized_value_label, std::string harmonic_drive_prefix, CCTools::ModelHandler &model_handler, double max_value) : AbstractOptimizer(true), optimized_value_label_(optimized_value_label)
{
    setup(model_handler, max_value, harmonic_drive_prefix);
}

void LinearOptimizer::setup(CCTools::ModelHandler &model_handler, double max_value, std::string harmonic_drive_prefix)
{
    harmonic_drive_prefix_ = harmonic_drive_prefix;
    model_handler_ = model_handler;
    initCalculator();
    initial_drive_values_ = initHarmonicDrives();
    max_value_ = max_value;
}

void LinearOptimizer::logResults()
{
    Logger::info("=== All harmonics have been optimized ===");
    Logger::info("User-specified margin was: " + std::to_string(max_value_));
    print_harmonic_drive_values(initial_drive_values_);
    log_vector(current_values_, optimized_value_label_);
}

double LinearOptimizer::getMaxHarmonicValue()
{
    return getUserInput("Enter the maximum absolute value for harmonic values", LINEAR_OPTIMIZER_DEFAULT_MAX_VALUE);
}

std::vector<double> &LinearOptimizer::getResults()
{
    return current_values_;
}

double LinearOptimizer::fitLinearGetRoot(const std::vector<std::pair<double, double>> &points)
{
    auto [slope, intercept] = StatisticalAnalysis::linearRegression(points);
    double root = -intercept / slope;

    return root;
}

void LinearOptimizer::getDriveValueAndType(const std::string &identifier, double &current_drive_value, CCTools::HarmonicDriveParameterType &drive_type)
{
    if (initial_drive_values_[identifier].isConstant())
    {
        current_drive_value = initial_drive_values_[identifier].getConstant();
        drive_type = CCTools::HarmonicDriveParameterType::Constant;
    }
    else if (initial_drive_values_[identifier].isSlope())
    {
        current_drive_value = initial_drive_values_[identifier].getSlope();
        drive_type = CCTools::HarmonicDriveParameterType::Slope;
    }
    else
    {
        throw std::logic_error("This Optimizer only optimizes custom harmonics with constant/linear scaling functions. The scaling function for " + identifier + " is neither.");
    }
}

void LinearOptimizer::optimize()
{
    Logger::info("== Starting " + optimized_value_label_ + " optimizer ==");

    const boost::filesystem::path temp_json_file_path = model_handler_.getTempJsonPath();

    // flag to check if all values are within the margin
    bool all_within_margin;

    // handler for handling the results of the harmonics calculation
    CCTools::HarmonicsDataHandler harmonics_handler;

    // get the current values
    calculator_.reload_and_calc_harmonics(temp_json_file_path, harmonics_handler);
    current_values_ = getValues(harmonics_handler);

    // optimize as long as not all values are within the margin
    do
    {
        all_within_margin = true;

        // optimize each harmonic drive value
        for (auto &harmonic : initial_drive_values_)
        {
            // get current drive value and type
            double current_drive_value;
            CCTools::HarmonicDriveParameterType drive_type;
            getDriveValueAndType(harmonic.first, current_drive_value, drive_type);

            // get the value for the component currently being optimized
            int component = std::stoi(harmonic.first.substr(1));
            double current_value = current_values_[component - 1];

            // if value is not optimized yet, do it
            if (std::abs(current_value) > max_value_)
            {
                all_within_margin = false;

                // log info
                Logger::info("Now optimizing harmonic " + harmonic.first + ". Current drive value is " + std::to_string(current_drive_value) + " with " + optimized_value_label_ + " value " + std::to_string(current_value));

                // collect all datapoints (x=drive value, y=value) for a regression
                std::vector<std::pair<double, double>> data_points;
                data_points.emplace_back(current_drive_value, current_value);

                // change a small step to get the second data point for the linear regression
                double step = 0.01 * current_drive_value;
                if (step == 0)
                    step = OPTIMIZER_FALLBACK_STEP;

                double new_drive_value = current_drive_value + step;

                if (std::isnan(new_drive_value))
                {
                    throw std::runtime_error("New drive value is NaN. This indicates that the model received some invalid drive values. Aborting optimization.");
                }

                model_handler_.setHarmonicDriveValue(harmonic.first, CCTools::HarmonicDriveParameters(new_drive_value, drive_type));

                calculator_.reload_and_calc_harmonics(temp_json_file_path, harmonics_handler);
                std::vector<double> new_values = getValues(harmonics_handler);
                double new_value = new_values[component - 1];
                Logger::info("Initial step yielded new " + optimized_value_label_ + " value: " + std::to_string(new_value) + " for new drive value: " + std::to_string(new_drive_value));

                data_points.emplace_back(new_drive_value, new_value);

                while (true)
                {
                    double optimized_drive_value = fitLinearGetRoot(data_points);

                    model_handler_.setHarmonicDriveValue(harmonic.first, CCTools::HarmonicDriveParameters(optimized_drive_value, drive_type));

                    calculator_.reload_and_calc_harmonics(temp_json_file_path, harmonics_handler);

                    std::vector<double> optimized_values = getValues(harmonics_handler);
                    double optimized_value = optimized_values[component - 1];
                    Logger::info("New " + optimized_value_label_ + " value: " + std::to_string(optimized_value) + " for new drive value: " + std::to_string(optimized_drive_value));

                    data_points.emplace_back(optimized_drive_value, optimized_value);

                    if (std::abs(optimized_value) <= max_value_ || data_points.size() >= LINEAR_OPTIMIZER_MAX_DATAPOINTS)
                    {
                        current_values_ = optimized_values;
                        harmonic.second.setValue(optimized_drive_value, drive_type);
                        if (data_points.size() >= LINEAR_OPTIMIZER_MAX_DATAPOINTS)
                        {
                            Logger::info("Optimizer moved on from " + harmonic.first + " after " + std::to_string(LINEAR_OPTIMIZER_MAX_DATAPOINTS) + " datapoints. This harmonic will be optimized in the next iteration.");
                        }
                        else
                        {
                            Logger::info("Optimized " + harmonic.first + " with drive value " + std::to_string(optimized_drive_value) + " and " + optimized_value_label_ + " value: " + std::to_string(optimized_value));
                        }
                        break;
                    }

                    current_drive_value = optimized_drive_value;
                    current_value = optimized_value;
                }
            }
        }
    } while (!all_within_margin);
}
