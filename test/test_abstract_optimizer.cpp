#include "gtest/gtest.h"
#include "bn_optimizer.h"
#include <boost/filesystem.hpp>
#include <constants.h>

// Define class that is not abstract and exposes some protected/private methods for testing
class TestAbstractOptimizer : public BnOptimizer
{
public:
    using BnOptimizer::BnOptimizer; // Inherit constructors

    using BnOptimizer::assertAllHarmonicsPresent;
    using BnOptimizer::assertOnlyLinearDrives;
    using BnOptimizer::computeMagnetEllBounds;
    using BnOptimizer::getMagnetLength;
    using BnOptimizer::getMainComponent;
};

class AbstractOptimizerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        CCTools::ModelHandler handler1(TEST_DATA_DIR + "quad_test_all_linear_noBinormal.json");
        opt_quad_alllinear = new TestAbstractOptimizer(handler1, 0.1);

        CCTools::ModelHandler handler2(TEST_DATA_DIR + "quad_test_noBinormal.json");
        opt_quad_test = new TestAbstractOptimizer(handler2, 0.1);

        CCTools::ModelHandler handler3(TEST_DATA_DIR + "quad_test_no10_noBinormal.json");
        opt_quad_test_no10 = new TestAbstractOptimizer(handler3, 0.1);

        CCTools::ModelHandler handler4(TEST_DATA_DIR + "quad_test_unoptimized.json");
        opt_quad_unoptimized = new TestAbstractOptimizer(handler4, 0.1);
    }

    void TearDown() override
    {
        delete opt_quad_alllinear;
        delete opt_quad_test;
        delete opt_quad_test_no10;
        delete opt_quad_unoptimized;
    }

    TestAbstractOptimizer *opt_quad_alllinear;
    TestAbstractOptimizer *opt_quad_test;
    TestAbstractOptimizer *opt_quad_test_no10;
    TestAbstractOptimizer *opt_quad_unoptimized;
};

TEST_F(AbstractOptimizerTest, getMagnetLength)
{
    opt_quad_alllinear->computeMagnetEllBounds();
    ASSERT_NEAR(240.41, opt_quad_alllinear->getMagnetLength(), 1e-2);

    opt_quad_test->computeMagnetEllBounds();
    ASSERT_NEAR(240.41, opt_quad_test->getMagnetLength(), 1e-2);
}

TEST_F(AbstractOptimizerTest, allHarmonicsPresent)
{
    // test assertAllHarmonics
    ASSERT_NO_THROW(opt_quad_alllinear->assertAllHarmonicsPresent(););
    ASSERT_NO_THROW(opt_quad_test->assertAllHarmonicsPresent(););
    ASSERT_THROW(opt_quad_test_no10->assertAllHarmonicsPresent();, std::runtime_error);

    // the above sets the main component
    ASSERT_EQ(2, opt_quad_alllinear->getMainComponent());
}

TEST_F(AbstractOptimizerTest, assertOnlyLinearDrives)
{
    // this model only has linears
    ASSERT_NO_THROW(opt_quad_alllinear->assertOnlyLinearDrives(););

    // this model has non-linear drives
    ASSERT_THROW(opt_quad_test->assertOnlyLinearDrives();, std::runtime_error);
}