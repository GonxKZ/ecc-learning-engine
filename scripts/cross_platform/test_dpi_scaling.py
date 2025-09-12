#!/usr/bin/env python3
"""
DPI Scaling Test Script

Tests DPI scaling functionality across different platforms and displays.
Validates high-DPI support, scaling factors, and UI element sizing.

Author: ECScope Development Team
Version: 1.0.0
"""

import os
import sys
import json
import time
import platform
import subprocess
import argparse
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass

@dataclass
class DisplayInfo:
    width: int
    height: int
    dpi: float
    scale_factor: float
    is_primary: bool
    name: str

@dataclass
class DPITestResult:
    display: DisplayInfo
    test_name: str
    expected_size: Tuple[int, int]
    actual_size: Tuple[int, int]
    passed: bool
    error_message: Optional[str] = None

class DPIScalingTester:
    def __init__(self, platform_name: str, artifacts_dir: Path):
        self.platform = platform_name
        self.artifacts_dir = artifacts_dir
        self.displays = []
        self.test_results = []
        
    def detect_displays(self) -> List[DisplayInfo]:
        """Detect available displays and their properties."""
        displays = []
        
        if self.platform == "windows":
            displays = self._detect_windows_displays()
        elif self.platform == "macos":
            displays = self._detect_macos_displays()
        elif self.platform == "linux":
            displays = self._detect_linux_displays()
        
        self.displays = displays
        return displays
    
    def _detect_windows_displays(self) -> List[DisplayInfo]:
        """Detect Windows displays using PowerShell."""
        try:
            # PowerShell command to get display info
            ps_command = """
            Get-WmiObject -Class Win32_DesktopMonitor | ForEach-Object {
                $monitor = $_
                $display = Get-WmiObject -Class Win32_VideoController | Where-Object { $_.DeviceID -eq $monitor.PNPDeviceID }
                [PSCustomObject]@{
                    Name = $monitor.Name
                    Width = $monitor.ScreenWidth
                    Height = $monitor.ScreenHeight
                    DPI = 96  # Default Windows DPI
                }
            } | ConvertTo-Json
            """
            
            result = subprocess.run(
                ["powershell", "-Command", ps_command],
                capture_output=True,
                text=True,
                check=True
            )
            
            display_data = json.loads(result.stdout)
            if not isinstance(display_data, list):
                display_data = [display_data]
            
            displays = []
            for i, display in enumerate(display_data):
                # Calculate scale factor (Windows typically uses 96 DPI as baseline)
                scale_factor = display.get('DPI', 96) / 96.0
                
                displays.append(DisplayInfo(
                    width=display.get('Width', 1920),
                    height=display.get('Height', 1080),
                    dpi=display.get('DPI', 96),
                    scale_factor=scale_factor,
                    is_primary=(i == 0),
                    name=display.get('Name', f'Display {i+1}')
                ))
            
            return displays
            
        except Exception as e:
            print(f"Warning: Could not detect Windows displays: {e}")
            # Fallback to default display
            return [DisplayInfo(
                width=1920, height=1080, dpi=96, scale_factor=1.0,
                is_primary=True, name="Default Display"
            )]
    
    def _detect_macos_displays(self) -> List[DisplayInfo]:
        """Detect macOS displays using system_profiler."""
        try:
            result = subprocess.run(
                ["system_profiler", "SPDisplaysDataType", "-json"],
                capture_output=True,
                text=True,
                check=True
            )
            
            display_data = json.loads(result.stdout)
            displays = []
            
            for display_group in display_data.get('SPDisplaysDataType', []):
                for display_key, display_info in display_group.items():
                    if isinstance(display_info, dict) and 'spdisplays_resolution' in display_info:
                        resolution = display_info['spdisplays_resolution']
                        # Parse resolution string like "1920 x 1080"
                        if ' x ' in resolution:
                            width, height = map(int, resolution.split(' x '))
                        else:
                            width, height = 1920, 1080
                        
                        # Check for Retina display
                        is_retina = 'spdisplays_retina' in display_info and display_info['spdisplays_retina'] == 'spdisplays_yes'
                        scale_factor = 2.0 if is_retina else 1.0
                        dpi = 220 if is_retina else 110  # Approximate DPI values
                        
                        displays.append(DisplayInfo(
                            width=width,
                            height=height,
                            dpi=dpi,
                            scale_factor=scale_factor,
                            is_primary=len(displays) == 0,
                            name=display_info.get('_name', f'Display {len(displays)+1}')
                        ))
            
            if not displays:
                # Fallback for macOS
                displays.append(DisplayInfo(
                    width=1920, height=1080, dpi=110, scale_factor=1.0,
                    is_primary=True, name="Default Display"
                ))
            
            return displays
            
        except Exception as e:
            print(f"Warning: Could not detect macOS displays: {e}")
            return [DisplayInfo(
                width=1920, height=1080, dpi=110, scale_factor=1.0,
                is_primary=True, name="Default Display"
            )]
    
    def _detect_linux_displays(self) -> List[DisplayInfo]:
        """Detect Linux displays using xrandr or other tools."""
        try:
            # Try xrandr first
            result = subprocess.run(
                ["xrandr", "--verbose"],
                capture_output=True,
                text=True,
                check=True
            )
            
            displays = []
            current_display = None
            
            for line in result.stdout.split('\n'):
                line = line.strip()
                
                # Look for connected displays
                if ' connected' in line:
                    parts = line.split()
                    display_name = parts[0]
                    
                    # Parse resolution
                    resolution_part = None
                    for part in parts:
                        if 'x' in part and '+' in part:
                            resolution_part = part.split('+')[0]
                            break
                    
                    if resolution_part and 'x' in resolution_part:
                        width, height = map(int, resolution_part.split('x'))
                    else:
                        width, height = 1920, 1080
                    
                    # Estimate DPI (Linux default is typically 96)
                    dpi = 96
                    scale_factor = 1.0
                    
                    # Check for high-DPI hints
                    if width >= 3840 or height >= 2160:  # 4K or higher
                        scale_factor = 2.0
                        dpi = 192
                    elif width >= 2560 or height >= 1440:  # QHD
                        scale_factor = 1.5
                        dpi = 144
                    
                    current_display = DisplayInfo(
                        width=width,
                        height=height,
                        dpi=dpi,
                        scale_factor=scale_factor,
                        is_primary='primary' in line,
                        name=display_name
                    )
                    displays.append(current_display)
            
            if not displays:
                # Fallback for Linux
                displays.append(DisplayInfo(
                    width=1920, height=1080, dpi=96, scale_factor=1.0,
                    is_primary=True, name="Default Display"
                ))
            
            return displays
            
        except Exception as e:
            print(f"Warning: Could not detect Linux displays using xrandr: {e}")
            
            # Try alternative method for Wayland
            try:
                # Check if we're on Wayland
                if os.environ.get('WAYLAND_DISPLAY'):
                    # Wayland detection is limited, use defaults
                    return [DisplayInfo(
                        width=1920, height=1080, dpi=96, scale_factor=1.0,
                        is_primary=True, name="Wayland Display"
                    )]
            except Exception:
                pass
            
            return [DisplayInfo(
                width=1920, height=1080, dpi=96, scale_factor=1.0,
                is_primary=True, name="Default Display"
            )]
    
    def run_dpi_test(self, test_executable: Path, display: DisplayInfo) -> List[DPITestResult]:
        """Run DPI scaling test on a specific display."""
        results = []
        
        if not test_executable.exists():
            print(f"Warning: Test executable not found: {test_executable}")
            return results
        
        # Set environment variables for the test
        env = os.environ.copy()
        env['ECSCOPE_TEST_DPI_SCALE'] = str(display.scale_factor)
        env['ECSCOPE_TEST_DISPLAY_WIDTH'] = str(display.width)
        env['ECSCOPE_TEST_DISPLAY_HEIGHT'] = str(display.height)
        
        if self.platform == "linux":
            # Set up display for Linux
            if 'DISPLAY' not in env:
                env['DISPLAY'] = ':0'
        
        try:
            print(f"Running DPI test on {display.name} (Scale: {display.scale_factor}x)")
            
            # Run the test with timeout
            result = subprocess.run(
                [str(test_executable), "--dpi-test", "--json-output"],
                capture_output=True,
                text=True,
                env=env,
                timeout=60  # 1 minute timeout
            )
            
            if result.returncode == 0 and result.stdout:
                # Parse JSON output from test
                try:
                    test_data = json.loads(result.stdout)
                    
                    for test_case in test_data.get('dpi_tests', []):
                        expected = tuple(test_case.get('expected_size', [0, 0]))
                        actual = tuple(test_case.get('actual_size', [0, 0]))
                        
                        # Check if sizes are within acceptable tolerance (5%)
                        tolerance = 0.05
                        width_diff = abs(actual[0] - expected[0]) / max(expected[0], 1)
                        height_diff = abs(actual[1] - expected[1]) / max(expected[1], 1)
                        
                        passed = width_diff <= tolerance and height_diff <= tolerance
                        
                        error_msg = None
                        if not passed:
                            error_msg = f"Size mismatch: expected {expected}, got {actual}"
                        
                        results.append(DPITestResult(
                            display=display,
                            test_name=test_case.get('name', 'Unknown Test'),
                            expected_size=expected,
                            actual_size=actual,
                            passed=passed,
                            error_message=error_msg
                        ))
                        
                except json.JSONDecodeError as e:
                    results.append(DPITestResult(
                        display=display,
                        test_name="JSON Parse",
                        expected_size=(0, 0),
                        actual_size=(0, 0),
                        passed=False,
                        error_message=f"Failed to parse JSON output: {e}"
                    ))
            else:
                results.append(DPITestResult(
                    display=display,
                    test_name="Test Execution",
                    expected_size=(0, 0),
                    actual_size=(0, 0),
                    passed=False,
                    error_message=f"Test failed with return code {result.returncode}: {result.stderr}"
                ))
        
        except subprocess.TimeoutExpired:
            results.append(DPITestResult(
                display=display,
                test_name="Test Timeout",
                expected_size=(0, 0),
                actual_size=(0, 0),
                passed=False,
                error_message="Test timed out after 60 seconds"
            ))
        except Exception as e:
            results.append(DPITestResult(
                display=display,
                test_name="Test Exception",
                expected_size=(0, 0),
                actual_size=(0, 0),
                passed=False,
                error_message=str(e)
            ))
        
        return results
    
    def run_all_tests(self) -> Dict:
        """Run DPI scaling tests on all detected displays."""
        print(f"Starting DPI scaling tests on {self.platform}")
        
        # Detect displays
        displays = self.detect_displays()
        print(f"Detected {len(displays)} display(s):")
        for display in displays:
            print(f"  - {display.name}: {display.width}x{display.height} @ {display.scale_factor}x scale")
        
        # Find test executable
        test_executable = self.artifacts_dir / "bin" / "ecscope_display_test"
        if self.platform == "windows":
            test_executable = test_executable.with_suffix(".exe")
        
        all_results = []
        
        for display in displays:
            display_results = self.run_dpi_test(test_executable, display)
            all_results.extend(display_results)
        
        self.test_results = all_results
        
        # Generate summary
        summary = {
            "platform": self.platform,
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "displays": [
                {
                    "name": d.name,
                    "width": d.width,
                    "height": d.height,
                    "dpi": d.dpi,
                    "scale_factor": d.scale_factor,
                    "is_primary": d.is_primary
                }
                for d in displays
            ],
            "test_results": [
                {
                    "display_name": r.display.name,
                    "test_name": r.test_name,
                    "expected_size": r.expected_size,
                    "actual_size": r.actual_size,
                    "passed": r.passed,
                    "error_message": r.error_message
                }
                for r in all_results
            ],
            "summary": {
                "total_tests": len(all_results),
                "passed_tests": sum(1 for r in all_results if r.passed),
                "failed_tests": sum(1 for r in all_results if not r.passed),
                "success_rate": (sum(1 for r in all_results if r.passed) / max(len(all_results), 1)) * 100
            }
        }
        
        return summary
    
    def generate_report(self, results: Dict, output_file: Path):
        """Generate DPI scaling test report."""
        # Save JSON report
        with open(output_file, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"\nDPI Scaling Test Report saved to: {output_file}")
        
        # Print summary to console
        summary = results['summary']
        print(f"\nDPI Scaling Test Summary:")
        print(f"  Platform: {results['platform']}")
        print(f"  Displays tested: {len(results['displays'])}")
        print(f"  Total tests: {summary['total_tests']}")
        print(f"  Passed: {summary['passed_tests']}")
        print(f"  Failed: {summary['failed_tests']}")
        print(f"  Success rate: {summary['success_rate']:.1f}%")
        
        # Show display details
        print(f"\nDisplay Details:")
        for display in results['displays']:
            print(f"  - {display['name']}: {display['width']}x{display['height']} @ {display['scale_factor']}x")
        
        # Show failed tests
        failed_tests = [r for r in results['test_results'] if not r['passed']]
        if failed_tests:
            print(f"\nFailed Tests:")
            for test in failed_tests:
                print(f"  - {test['display_name']}: {test['test_name']} - {test['error_message']}")
        else:
            print(f"\nAll DPI scaling tests passed!")

def main():
    parser = argparse.ArgumentParser(
        description="ECScope DPI Scaling Test Script"
    )
    parser.add_argument(
        "--platform",
        required=True,
        choices=["windows", "linux", "macos"],
        help="Target platform"
    )
    parser.add_argument(
        "--artifacts-dir",
        type=Path,
        required=True,
        help="Directory containing build artifacts"
    )
    parser.add_argument(
        "--output",
        type=Path,
        default="dpi_scaling_results.json",
        help="Output file for test results"
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable verbose output"
    )
    
    args = parser.parse_args()
    
    if not args.artifacts_dir.exists():
        print(f"Error: Artifacts directory not found: {args.artifacts_dir}")
        sys.exit(1)
    
    # Set up logging
    if args.verbose:
        import logging
        logging.basicConfig(level=logging.DEBUG)
    
    # Run DPI scaling tests
    tester = DPIScalingTester(args.platform, args.artifacts_dir)
    results = tester.run_all_tests()
    tester.generate_report(results, args.output)
    
    # Exit with error code if tests failed
    if results['summary']['failed_tests'] > 0:
        sys.exit(1)

if __name__ == "__main__":
    main()