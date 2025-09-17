# Testing FlockMTL
## Overview
To run the tests for this extension, simply run the CMake target titled "flock_tests". These tests ensure that flock components are functioning properly.

## Fusion Algorithm Tests
### Common
The extension offers a rank-based fusion algorithm (`fusion_rrf`) and score-based ranking algorithms (`fusion_combsum`, `fusion_combmnz`, `fusion_combmed`, and `fusion_combanz`). The following inputs are tested for all fusion algorithm:
* 1 row with 2 values/columns
* 5 rows with 2 values/columns
* 5 rows with 3 values/columns

### Rank-Based Algorithms only
* 5 rows with 2 columns, where one column contains normal ranks and the other column only contains ranks of 1

### Score-Based Algorithms only
* 5 rows with 2 columns, where some of the values are NULL/NaN
* 5 rows with 2 columns, with an entire NULL/NaN column
* 5 rows with 2 columns, with only NULL/NaN values in both columns