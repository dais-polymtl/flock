# Detailed Setup Guide for Flock with CLion

1. **Clone the Repository**

    ```bash
    git clone --recurse-submodules https://github.com/dais-polymtl/flock.git
    cd flock
    ```

2. **Set Up vcpkg**

    ```bash
    source scripts/setup_vcpkg.sh
    ```
   
3. **Open the Project in CLion**
    - Launch CLion and click **Open Project**.
    - Navigate to the `CMakeLists.txt` file located in the `duckdb` folder within the cloned repository. Use the following pattern:`flock/duckdb/CMakeLists.txt`.

4. **Configure CMake Options**
    - Open the CMake configuration settings in CLion.
    - Add the following options under **CMake Options**:

        ```text
        -DEXTENSION_STATIC_BUILD=1 -DDUCKDB_EXTENSION_CONFIGS=../extension_config.cmake -DVCPKG_MANIFEST_DIR=../../ -DVCPKG_BUILD=1 -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
        ```

    - Set the **Build Directory** to:

        ```text
        ../build/debug
        ```

5. **Adding a Release Profile**
    - In the CMake configuration settings, click the **+** button to add a new build profile named **Release**.
    - Configure the **CMake Options** and **Build Directory** in the same way as the Debug profile.
    - The result should look like this:
    ![assets/clion/image%207.png](assets/clion/image%207.png)

6. **Apply Changes and Wait for Setup**
    - Click **OK** to apply changes.
    - CLion will initialize the project. This process may take some time.

7. **Adjust Project Root Folder**

    After setup is complete, the project root folder will automatically shift to the extension root.
    - **Note**: make sure the file explorer is set to **Project Files**

8. **Set Up Execution**
    - Open **Run/Debug Configurations** in CLion.
    - Search for `shell`.
    - Click **Edit** to modify the shell configuration.
    - Enable **Emulate terminal in the output console** to make the shell interactive.

9. **Save Changes and Begin Coding**
   - Apply all changes, and your development environment is ready for Flock.
   - Enjoy coding! 😉
