# CCT-Harmonics-Optimizer

This software is designed to automatically optimize harmonic drive parameters for canted-cosine-theta (CCT) magnets in order to achieve perfect magnetic field quality. Three optimizers are provided to achieve different objectives. This project is part of the FCC-ee HTS4 research project at CERN.

## Overview

This software utilizes the [CCTools](https://github.com/olekuhlmann/CCTools) library, which is built on top of the open-source [RAT-Library](https://rat-gui.com/library.html).

## Documentation
The documentation of this application can be viewed [here](https://olekuhlmann.github.io/CCT-Harmonics-Optimizer). To run the software with an example model, follow the Installation and Example sections below.

## Installation

### Prerequisites

**Note: This software was made for Linux.**

Follow the [Prerequisites section](https://github.com/olekuhlmann/CCTools/blob/main/README.md#prerequisites) from CCTools to install required dependencies. Additionally, install the dependencies listed below.

1. **Eigen3**
    - Eigen3 is a library used for linear algebra computations
    ```sh
    sudo apt-get install libeigen3-dev
    ```


### Building and Running the Software

1. Clone this repository and build the software:
    ```sh
    git clone https://github.com/olekuhlmann/CCT-Harmonics-Optimizer.git
    cd cct-harmonics-optimizer
    mkdir build
    cd build
    cmake ..
    make
    ```

2. Run all tests:
    ```sh
    ./bin/runTests
    ```

3. To run the program:
    ```sh
    ./bin/main
    ```

## Theoretical Background

The magnetic field produced by electromagnets can be described by a series of harmonics, denoted as B1, B2, ..., B10. 
In this context, B1 represents the dipole field, B2 represents the quadrupole field, and so on up to the decapole field (B10). We omit all Bn for n > 10 here since they are not relevant for this use case.

When modeling CCT magnets using the RAT Library, custom harmonic components can be defined to fine-tune the magnetic field. These custom components allow for precise control over the magnetic field distribution. 

Each custom component can be characterized by its scaling function.
Custom harmonic components in the CCT model have a specific number of poles (X) and are associated primarily with the harmonic BX. When simulating the magnet and computing harmonics, we are interested in a few properties for each harmonic:

- **Bn Curve**: The Bn curve represents the magnitude of the harmonic along the length of the magnet, measured in Tesla (T).
- **bn Value**: The bn value is the (normalized) integral of the Bn curve over the length of the magnet. It indicates the overall magnitude of the harmonic component. The bn values are normalized, setting the largest one to 10,000 and all others relative to it.

In addition to the B components, we also consider the A components when analyzing the magnetic field:

- **An Curve**: The An curve represents the phase angle of the harmonic along the length of the magnet (i.e., the change in the harmonic's direction). The An curve provides insight into how the harmonic component rotates or shifts as it propagates along the magnet's length.
- **an Value**: The an value is the (normalized) integral of the An curve over the length of the magnet. It quantifies the overall phase shift or rotation of the harmonic component. The an values are normalized in the same way as the bn ones.

Together, the Bn and An curves, along with their respective bn and an values, provide a comprehensive understanding of the magnetic field characteristics.



## Usage
The optimizer requires a JSON file of a CCT magnet, created by the [RAT-GUI software](https://rat-gui.com/index.html) or [RAT Library](https://rat-gui.com/library.html). Ensure that the JSON file meets the following criteria:

- The JSON file should contain a harmonics calculation in the calculation tree. The optimizer will use the first harmonics calculation found in the tree.
    - The harmonics calculation's axis should span across the entire length of the magnet (or more). The optimizers will truncate the calculation's data to only consider the length of the magnet where applicable.
    - The axis (and therefore the magnet) must span across the z-axis. This is the default in RAT.
- The JSON file should contain a mesh calculation in the calculation tree. The optimizer will use the first mesh calculation found.

- The JSON file should include custom CCT harmonics for all harmonics B1 to B10 (except for the main harmonic, e.g., B2 for a quadrupole) in the model tree. These custom harmonics will be optimized by the software.
    - The custom harmonics should be named B1, B2, ...
    - If there is a custom component for the main harmonic, it cannot be named B1/B2/...
    - The custom harmonics must have an 'amplitude' of 'constant' or 'linear' (further restrictions to this apply depending on the optimizer used).
    - All custom harmonics with the same number of poles should have the same name and scaling function parameters, ensuring they are optimized together (e.g. when there are different custom harmonics for the different layers of a CCT).
- If the an Optimizer is to be used, skew harmonic components A1 to A10 need to be included with the same restrictions as the B harmonics. Additionally, these restrictions apply:
    - Contrary to the B components, the skew harmonic component for the main component (e.g., A2 for a quadrupole) needs to be included here as well and named accordingly (e.g., 'A2').
    - All skew harmonic components need to have an 'amplitude' of 'constant'.
    


Place the JSON file of the magnet in the `data` directory. The original JSON file will not be modified by the software. All changes are made to a copy of the original file.

Detailed logs of every run of the software are saved in the `logs` directory.

To run the software, use the command specified above ```./bin/main``` and select the desired optimizer. Further steps might be required depending on the optimizer chosen.

### Example

An example model `cct.json` can be found in the `examples` directory. To test this software, follow these steps:

1. Follow the installation steps outlined in this README
2. Place the `cct.json` file in the `data` directory.
3. Run the program:
    ```sh
    ./bin/main
    ```
4. Follow the prompts to select the correct JSON file, select the bn Optimizer, and enter 0.1 as the maximum absolute bn value.

The program will terminate after some minutes, providing the optimal parameters.

## Optimizers

### bn Optimizer

**Objective**

The goal of this optimizer is to adjust the custom harmonics so that the bn values for all harmonics are as near to 0 as possible. The main harmonic will always stay at 10,000.

**Background**

This optimizer considers custom harmonics with an 'amplitude' of 'constant' or 'linear'. For constant scaling functions, the 'constant' parameter will be optimized. For linear ones, the 'slope' parameter will be optimized. 
The relationship between the respective scaling function parameter of a custom harmonic and the bn value of that harmonic is almost linear, allowing for a fairly simple optimization.

**Approach**

One custom harmonic is optimized at a time. To get the bn value of this harmonic close to 0, this is the approach:

1. The optimizer performs harmonic calculations for at least two different values of the scaling function parameter (constant or slope).
2. A linear regression is applied to extrapolate the optimal parameter value that results in a bn value of 0.

In a round, all harmonics B1 to B10 are optimized once using this procedure, except for the main one.

Since the relationship between the scaling function constant and the respective bn value is not perfectly linear and the custom harmonic with X poles does not only influence the harmonic BX but also the others, the optimizer runs multiple rounds.
The optimizer will run these rounds until all harmonic bn values are below the user-specified margin. The default margin is 0.1. Caution is advised with lower values as runtime might explode.

**Limitations**

For some magnets, the optimal configuration (with all bn values 0) breaks the magnet (e.g., by increasing length of the magnet by a few orders of magnitude). In this case, the optimizer will try to approach the optimal solution as close as possible. This can be solved by changing the magnet model manually.

In certain cases of this, the optimizer might not terminate as it continuously gets closer to the optimal solution, never reaching it.

**Example**

Below is an example result of the bn Optimizer on a quadrupole model. The left are the harmonics before the optimizer and the right are the ones after. A threshold of 0.005 was used. All bn values were optimized below that threshold. The runtime was 1 minute on a machine with CUDA and a RTX 3080Ti.

<div align="center">
  <img width="684" alt="Before vs. After for an example of the bn Optimizer" src="https://github.com/user-attachments/assets/3ce508e4-431e-4a77-be41-1ec4fdaae5fe">
</div>

### an Optimizer

**Objective**

Similarly to the bn Optimizer, the goal of this optimizer is to adjust the custom harmonics so that the an values for all harmonics are as near to 0 as possible. The main harmonic will also be brought to 0.

**Background**

This optimizer works in the same way as the bn Optimizer. Instead of modifying the B harmonics to get low bn values, it modifies the A harmonics to get low an values.

The same background, approach & limitations apply.

**Example**

Below is an example result of the an Optimizer on a quadrupole model. The left are the harmonics before the optimizer and the right are the ones after. A threshold of 0.01 was used. All an values were optimized below that threshold. The runtime was 1.5 minutes on a machine with CUDA and a RTX 3080Ti.

<div align="center">
<img width="684" alt="Before vs. After for an example of the an Optimizer" src="https://github.com/user-attachments/assets/615651f6-7d9f-44f9-a9b0-6d2c1bdd5e14">
</div>

### Grid Search Optimizer

**Objective**

Similarly to the bn Optimizer, this optimizer also adjusts the custom harmonics to bring the bn values to 0. Additionally, it optimizes the custom harmonics to obtain Bn curves with a favorable shape. 

**Background**

This optimizer requires all custom harmonics to have an 'amplitude' of 'linear'. Both the 'offset' and 'slope' parameters of the scaling function will be optimized.
Again, the relationship between these two paramaters and the respective bn value is almost linear. 

"Favorable" Bn curves are those that have a shape as constant as possible, indicating that the magnitude of the harmonic does not significantly change over the length. 

This is characterized numerically by fitting a linear function to the Bn curve and considering the slope of this fitted function. When this slope is near 0, the linear function is almost constant. This suggests that the Bn curve is somewhat constant, indicating that its shape is favorable. 

The relationship between the two scaling function parameters and the slope of this fitted function is also almost linear.

**Approach**

Similarly to the bn Optimizer, this optimizer considers a single harmonic at a time and performs several rounds of optimizing all harmonics to reach the optimal configuration.

To optimize one harmonic, this is the approach:

1. Run a grid search in the space of the two scaling function parameters, i.e., simulate the magnet with numerous different scaling function parameters for the harmonic to be optimized.
2. In the 3D space of [offset, slope, bn value] and [offset, slope, slope of fitted function], fit a 2D plane to the data. This is possible since both relationships are almost linear.
3. From these 2D planes, extract the linear function where the respective plane has a bn value / slope of fitted function of 0.
4. Compute the intersection of these linear functions to extrapolate the optimal configuration.

**Limitations**

The limitations of the bn Optimizer apply here as well.

**Example**

Below is an example result of the Grid Search Optimizer on a quadrupole model. The left are the harmonics before the optimizer and the right are the ones after. All bn values were optimized close to 0 and all Bn curves show favorable shapes. The runtime was 60 minutes on a machine with CUDA and a RTX 3080Ti. For larger magnet models, the runtime can reach 5 hours with the same hardware.

<div align="center">
<img width="684" alt="Before vs. After for an example of the Grid Search Optimizer" src="https://github.com/user-attachments/assets/1707c8a1-4bc0-4103-95d7-ad71efcd180d">
</div>

## Author

Ole Kuhlmann  
Email: [ole.kuhlmann@rwth-aachen.de](mailto:ole.kuhlmann@rwth-aachen.de)  
GitHub: [olekuhlmann](https://github.com/olekuhlmann)

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
