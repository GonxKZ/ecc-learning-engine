#!/usr/bin/env python3
"""
ECScope Build System Validation Script
=====================================

This script validates the ECScope build system by testing various configurations
and ensuring all components work correctly together.
"""

import os
import sys
import subprocess
import tempfile
import shutil
from pathlib import Path
from typing import Dict, List, Tuple, Optional

class BuildSystemValidator:
    """Comprehensive validation of ECScope build system"""
    
    def __init__(self):
        self.project_root = Path(__file__).parent.absolute()
        self.validation_results = []
        self.errors = []
        
    def log_result(self, test_name: str, passed: bool, message: str = ""):
        """Log a test result"""
        status = "✅ PASS" if passed else "❌ FAIL"
        full_message = f"{status}: {test_name}"
        if message:
            full_message += f" - {message}"
        
        print(full_message)
        self.validation_results.append({
            'test': test_name,
            'passed': passed,
            'message': message
        })
        
        if not passed:
            self.errors.append(full_message)
    
    def run_command(self, cmd: List[str], cwd: Optional[Path] = None, 
                   timeout: int = 300) -> Tuple[bool, str, str]:
        """Run a command and return success status and output"""
        try:
            result = subprocess.run(
                cmd, cwd=cwd, capture_output=True, text=True, 
                timeout=timeout, check=False
            )
            return result.returncode == 0, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return False, "", "Command timed out"
        except Exception as e:
            return False, "", str(e)
    
    def check_cmake_files(self) -> bool:
        """Validate CMake file structure and syntax"""
        print("\n🔍 Validating CMake Files...")
        
        # Check main CMakeLists.txt
        main_cmake = self.project_root / "CMakeLists.txt"
        if not main_cmake.exists():
            self.log_result("Main CMakeLists.txt exists", False, "File not found")
            return False
        
        self.log_result("Main CMakeLists.txt exists", True)
        
        # Check cmake modules
        cmake_modules = [
            "cmake/ECScope.cmake",
            "cmake/Dependencies.cmake", 
            "cmake/Platform.cmake",
            "cmake/Testing.cmake",
            "cmake/Installation.cmake"
        ]
        
        all_modules_exist = True
        for module in cmake_modules:
            module_path = self.project_root / module
            exists = module_path.exists()
            self.log_result(f"Module {module} exists", exists)
            if not exists:
                all_modules_exist = False
        
        # Check for syntax errors by running CMake configure in dry-run mode
        with tempfile.TemporaryDirectory() as temp_dir:
            success, stdout, stderr = self.run_command([
                "cmake", str(self.project_root), "-B", temp_dir, "-N"
            ])
            self.log_result("CMake syntax validation", success, stderr[:100] if stderr else "")
        
        return all_modules_exist and success
    
    def test_basic_configuration(self) -> bool:
        """Test basic CMake configuration without building"""
        print("\n🔧 Testing Basic Configuration...")
        
        configs_to_test = [
            ("Debug", {"CMAKE_BUILD_TYPE": "Debug"}),
            ("Release", {"CMAKE_BUILD_TYPE": "Release"}),
            ("Educational", {
                "CMAKE_BUILD_TYPE": "RelWithDebInfo",
                "ECSCOPE_EDUCATIONAL_MODE": "ON"
            })
        ]
        
        all_passed = True
        for config_name, config_options in configs_to_test:
            with tempfile.TemporaryDirectory() as temp_dir:
                cmake_args = ["cmake", str(self.project_root), "-B", temp_dir]
                for key, value in config_options.items():
                    cmake_args.append(f"-D{key}={value}")
                
                success, stdout, stderr = self.run_command(cmake_args)
                self.log_result(f"{config_name} configuration", success, 
                               stderr[:100] if stderr else "")
                
                if not success:
                    all_passed = False
        
        return all_passed
    
    def test_feature_flags(self) -> bool:
        """Test various feature flag combinations"""
        print("\n🎛️  Testing Feature Flags...")
        
        feature_combinations = [
            # Minimal build
            {
                "ECSCOPE_ENABLE_GRAPHICS": "OFF",
                "ECSCOPE_ENABLE_SCRIPTING": "OFF",
                "ECSCOPE_BUILD_TESTS": "OFF",
                "ECSCOPE_BUILD_EXAMPLES": "OFF"
            },
            # Graphics build
            {
                "ECSCOPE_ENABLE_GRAPHICS": "ON",
                "ECSCOPE_ENABLE_SIMD": "ON",
                "ECSCOPE_BUILD_EXAMPLES": "ON"
            },
            # Full feature build
            {
                "ECSCOPE_ENABLE_SIMD": "ON",
                "ECSCOPE_ENABLE_JOB_SYSTEM": "ON",
                "ECSCOPE_ENABLE_3D_PHYSICS": "ON",
                "ECSCOPE_ENABLE_HARDWARE_DETECTION": "ON",
                "ECSCOPE_BUILD_TESTS": "ON"
            }
        ]
        
        all_passed = True
        for i, features in enumerate(feature_combinations, 1):
            with tempfile.TemporaryDirectory() as temp_dir:
                cmake_args = ["cmake", str(self.project_root), "-B", temp_dir]
                cmake_args.append("-DCMAKE_BUILD_TYPE=Release")
                
                for key, value in features.items():
                    cmake_args.append(f"-D{key}={value}")
                
                success, stdout, stderr = self.run_command(cmake_args)
                self.log_result(f"Feature combination {i}", success,
                               stderr[:100] if stderr else "")
                
                if not success:
                    all_passed = False
        
        return all_passed
    
    def test_dependency_resolution(self) -> bool:
        """Test dependency finding and resolution"""
        print("\n📦 Testing Dependency Resolution...")
        
        # Test with dependencies disabled
        with tempfile.TemporaryDirectory() as temp_dir:
            cmake_args = [
                "cmake", str(self.project_root), "-B", temp_dir,
                "-DCMAKE_BUILD_TYPE=Release",
                "-DECSCOPE_ENABLE_GRAPHICS=OFF",
                "-DECSCOPE_ENABLE_SCRIPTING=OFF"
            ]
            
            success, stdout, stderr = self.run_command(cmake_args)
            self.log_result("Minimal dependencies", success,
                           stderr[:100] if stderr else "")
        
        return success
    
    def test_platform_detection(self) -> bool:
        """Test platform detection and configuration"""
        print("\n🖥️  Testing Platform Detection...")
        
        with tempfile.TemporaryDirectory() as temp_dir:
            cmake_args = [
                "cmake", str(self.project_root), "-B", temp_dir,
                "-DCMAKE_BUILD_TYPE=Release",
                "-DECSCOPE_ENABLE_HARDWARE_DETECTION=ON"
            ]
            
            success, stdout, stderr = self.run_command(cmake_args)
            
            # Check if platform was detected
            platform_detected = any([
                "ECSCOPE_PLATFORM_WINDOWS" in stdout,
                "ECSCOPE_PLATFORM_LINUX" in stdout,
                "ECSCOPE_PLATFORM_MACOS" in stdout
            ])
            
            self.log_result("Platform detection", platform_detected and success)
            
        return platform_detected and success
    
    def test_compiler_support(self) -> bool:
        """Test compiler-specific configurations"""
        print("\n🔧 Testing Compiler Support...")
        
        # Test with different optimization levels
        optimization_tests = [
            ("O0", "-DCMAKE_CXX_FLAGS=-O0"),
            ("O2", "-DCMAKE_CXX_FLAGS=-O2"),
            ("O3", "-DCMAKE_CXX_FLAGS=-O3")
        ]
        
        all_passed = True
        for opt_name, opt_flag in optimization_tests:
            with tempfile.TemporaryDirectory() as temp_dir:
                cmake_args = [
                    "cmake", str(self.project_root), "-B", temp_dir,
                    "-DCMAKE_BUILD_TYPE=Release",
                    opt_flag
                ]
                
                success, stdout, stderr = self.run_command(cmake_args)
                self.log_result(f"Optimization {opt_name}", success,
                               stderr[:100] if stderr else "")
                
                if not success:
                    all_passed = False
        
        return all_passed
    
    def test_build_system_generators(self) -> bool:
        """Test different CMake generators"""
        print("\n⚙️  Testing Build System Generators...")
        
        # Test generators available on current platform
        generators_to_test = []
        
        # Check for Ninja
        if self.command_exists("ninja"):
            generators_to_test.append("Ninja")
        
        # Check for Make (Unix-like systems)
        if self.command_exists("make"):
            generators_to_test.append("Unix Makefiles")
        
        # Visual Studio on Windows
        if sys.platform == "win32":
            generators_to_test.append("Visual Studio 16 2019")
        
        if not generators_to_test:
            self.log_result("Build generators", False, "No supported generators found")
            return False
        
        all_passed = True
        for generator in generators_to_test:
            with tempfile.TemporaryDirectory() as temp_dir:
                cmake_args = [
                    "cmake", str(self.project_root), "-B", temp_dir,
                    "-G", generator,
                    "-DCMAKE_BUILD_TYPE=Release"
                ]
                
                success, stdout, stderr = self.run_command(cmake_args)
                self.log_result(f"Generator {generator}", success,
                               stderr[:100] if stderr else "")
                
                if not success:
                    all_passed = False
        
        return all_passed
    
    def test_installation_configuration(self) -> bool:
        """Test installation and packaging configuration"""
        print("\n📦 Testing Installation Configuration...")
        
        with tempfile.TemporaryDirectory() as temp_dir:
            install_dir = Path(temp_dir) / "install"
            
            cmake_args = [
                "cmake", str(self.project_root), "-B", temp_dir,
                "-DCMAKE_BUILD_TYPE=Release",
                f"-DCMAKE_INSTALL_PREFIX={install_dir}",
                "-DECSCOPE_BUILD_TESTS=OFF",  # Skip tests for faster config
                "-DECSCOPE_BUILD_EXAMPLES=OFF"
            ]
            
            success, stdout, stderr = self.run_command(cmake_args)
            
            if success:
                # Check if install target was created
                success, stdout, stderr = self.run_command([
                    "cmake", "--build", temp_dir, "--target", "help"
                ])
                
                install_target_exists = "install" in stdout
                self.log_result("Installation target created", install_target_exists)
                success = success and install_target_exists
            else:
                self.log_result("Installation configuration", False,
                               stderr[:100] if stderr else "")
        
        return success
    
    def test_docker_configuration(self) -> bool:
        """Test Docker configuration files"""
        print("\n🐳 Testing Docker Configuration...")
        
        docker_files = [
            "cmake/docker/Dockerfile.ubuntu.in",
            "cmake/docker/Dockerfile.alpine.in", 
            "cmake/docker/Dockerfile.multistage.in",
            "cmake/docker/docker-compose.yml.in"
        ]
        
        all_exist = True
        for docker_file in docker_files:
            file_path = self.project_root / docker_file
            exists = file_path.exists()
            self.log_result(f"Docker file {docker_file}", exists)
            if not exists:
                all_exist = False
        
        return all_exist
    
    def test_helper_scripts(self) -> bool:
        """Test helper scripts and build automation"""
        print("\n🔧 Testing Helper Scripts...")
        
        # Test build.py script exists and has valid syntax
        build_script = self.project_root / "build.py"
        if not build_script.exists():
            self.log_result("build.py exists", False)
            return False
        
        self.log_result("build.py exists", True)
        
        # Test Python syntax
        success, stdout, stderr = self.run_command([
            sys.executable, "-m", "py_compile", str(build_script)
        ])
        self.log_result("build.py syntax", success, stderr[:100] if stderr else "")
        
        # Test help output
        success, stdout, stderr = self.run_command([
            sys.executable, str(build_script), "--help"
        ])
        help_works = success and "ECScope Advanced Build System" in stdout
        self.log_result("build.py help", help_works)
        
        return success and help_works
    
    def command_exists(self, command: str) -> bool:
        """Check if a command exists"""
        try:
            subprocess.run([command, "--version"], 
                         capture_output=True, check=True, timeout=5)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError, subprocess.TimeoutExpired):
            return False
    
    def run_full_validation(self) -> bool:
        """Run complete build system validation"""
        print("🚀 ECScope Build System Validation")
        print("=" * 50)
        
        validation_steps = [
            ("CMake Files", self.check_cmake_files),
            ("Basic Configuration", self.test_basic_configuration),
            ("Feature Flags", self.test_feature_flags),
            ("Dependency Resolution", self.test_dependency_resolution),
            ("Platform Detection", self.test_platform_detection),
            ("Compiler Support", self.test_compiler_support),
            ("Build Generators", self.test_build_system_generators),
            ("Installation Config", self.test_installation_configuration),
            ("Docker Configuration", self.test_docker_configuration),
            ("Helper Scripts", self.test_helper_scripts)
        ]
        
        all_passed = True
        for step_name, step_function in validation_steps:
            try:
                result = step_function()
                if not result:
                    all_passed = False
            except Exception as e:
                self.log_result(f"{step_name} (Exception)", False, str(e))
                all_passed = False
        
        # Print summary
        print("\n" + "=" * 50)
        print("📊 VALIDATION SUMMARY")
        print("=" * 50)
        
        passed = sum(1 for r in self.validation_results if r['passed'])
        total = len(self.validation_results)
        
        print(f"Tests Passed: {passed}/{total}")
        print(f"Success Rate: {(passed/total)*100:.1f}%")
        
        if self.errors:
            print(f"\n❌ FAILED TESTS ({len(self.errors)}):")
            for error in self.errors:
                print(f"  {error}")
        
        if all_passed:
            print("\n✅ BUILD SYSTEM VALIDATION SUCCESSFUL!")
            print("ECScope build system is ready for educational and production use.")
        else:
            print("\n❌ BUILD SYSTEM VALIDATION FAILED!")
            print("Some components need attention before the build system is ready.")
        
        return all_passed
    
    def run_quick_validation(self) -> bool:
        """Run essential validation checks only"""
        print("⚡ ECScope Build System Quick Validation")
        print("=" * 40)
        
        essential_checks = [
            ("CMake Files", self.check_cmake_files),
            ("Basic Configuration", self.test_basic_configuration),
            ("Helper Scripts", self.test_helper_scripts)
        ]
        
        all_passed = True
        for step_name, step_function in essential_checks:
            try:
                result = step_function()
                if not result:
                    all_passed = False
            except Exception as e:
                self.log_result(f"{step_name} (Exception)", False, str(e))
                all_passed = False
        
        print("\n" + "=" * 40)
        if all_passed:
            print("✅ QUICK VALIDATION SUCCESSFUL!")
        else:
            print("❌ QUICK VALIDATION FAILED!")
        
        return all_passed


def main():
    import argparse
    
    parser = argparse.ArgumentParser(
        description="Validate ECScope Build System",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument("--quick", "-q", action="store_true",
                       help="Run only essential validation checks")
    parser.add_argument("--verbose", "-v", action="store_true",
                       help="Enable verbose output")
    
    args = parser.parse_args()
    
    validator = BuildSystemValidator()
    
    try:
        if args.quick:
            success = validator.run_quick_validation()
        else:
            success = validator.run_full_validation()
        
        return 0 if success else 1
    
    except KeyboardInterrupt:
        print("\n❌ Validation interrupted by user")
        return 1
    except Exception as e:
        print(f"\n❌ Validation failed with error: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())