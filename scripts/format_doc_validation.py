#!/usr/bin/env python3
"""
Documentation Validation Report Formatter for ECScope
Formats documentation validation results into readable markdown.
"""

import json
import argparse
import sys
from pathlib import Path
from typing import Dict, Any

def format_validation_report(validation_data: Dict[str, Any]) -> str:
    """Format validation data into markdown report."""
    
    report_lines = []
    
    # Header
    status_icon = "✅" if validation_data['status'] == 'passed' else "❌"
    report_lines.append(f"{status_icon} **Status:** {validation_data['status'].upper()}")
    report_lines.append("")
    
    # Statistics
    stats = validation_data['statistics']
    report_lines.append("### 📊 Statistics")
    report_lines.append(f"- Files checked: **{stats['files_checked']}**")
    report_lines.append(f"- Examples validated: **{stats['examples_validated']}**")
    report_lines.append(f"- Missing documentation: **{stats['missing_docs']}**")
    report_lines.append(f"- Outdated examples: **{stats['outdated_examples']}**")
    report_lines.append("")
    
    # Summary
    summary = validation_data['summary']
    report_lines.append("### 📋 Summary")
    report_lines.append(f"- Total issues: **{summary['total_issues']}**")
    report_lines.append(f"- Total warnings: **{summary['total_warnings']}**")
    
    if summary['total_issues'] > 0:
        report_lines.append(f"- Files with issues: **{summary['files_with_issues']}**")
    if summary['total_warnings'] > 0:
        report_lines.append(f"- Files with warnings: **{summary['files_with_warnings']}**")
    
    report_lines.append("")
    
    # Issues
    if validation_data['issues']:
        report_lines.append("### ❌ Issues")
        
        # Group issues by type
        issues_by_type = {}
        for issue in validation_data['issues']:
            issue_type = issue['type']
            if issue_type not in issues_by_type:
                issues_by_type[issue_type] = []
            issues_by_type[issue_type].append(issue)
        
        for issue_type, issues in issues_by_type.items():
            report_lines.append(f"#### {issue_type.replace('_', ' ').title()}")
            
            for issue in issues[:10]:  # Limit to first 10 per type
                file_name = Path(issue['file']).name
                report_lines.append(f"- **{file_name}**: {issue['message']}")
                
            if len(issues) > 10:
                report_lines.append(f"- ... and {len(issues) - 10} more files")
                
            report_lines.append("")
    
    # Warnings
    if validation_data['warnings']:
        report_lines.append("### ⚠️ Warnings")
        
        # Group warnings by type
        warnings_by_type = {}
        for warning in validation_data['warnings']:
            warning_type = warning['type']
            if warning_type not in warnings_by_type:
                warnings_by_type[warning_type] = []
            warnings_by_type[warning_type].append(warning)
        
        for warning_type, warnings in warnings_by_type.items():
            report_lines.append(f"#### {warning_type.replace('_', ' ').title()}")
            
            for warning in warnings[:5]:  # Limit to first 5 per type
                file_name = Path(warning['file']).name
                report_lines.append(f"- **{file_name}**: {warning['message']}")
                
                # Add details if available
                if 'details' in warning and warning['details']:
                    for detail in warning['details'][:2]:  # Show max 2 details
                        report_lines.append(f"  - `{detail.strip()}`")
                
            if len(warnings) > 5:
                report_lines.append(f"- ... and {len(warnings) - 5} more files")
                
            report_lines.append("")
    
    # Recommendations
    if validation_data['issues'] or validation_data['warnings']:
        report_lines.append("### 💡 Recommendations")
        
        if stats['missing_docs'] > 0:
            report_lines.append("- Add `@brief` documentation to public API headers")
        
        if stats['outdated_examples'] > 0:
            report_lines.append("- Update examples to use current ECScope API")
            
        if any(issue['type'] == 'incomplete_example' for issue in validation_data['issues']):
            report_lines.append("- Ensure all example files have proper main functions and includes")
            
        report_lines.append("- Consider adding more comprehensive documentation comments")
        report_lines.append("- Review and resolve TODO/FIXME comments in public headers")
    else:
        report_lines.append("### 🎉 Great Job!")
        report_lines.append("Documentation validation passed without issues!")
    
    return "\n".join(report_lines)

def main():
    parser = argparse.ArgumentParser(description="Format documentation validation report")
    parser.add_argument("validation_file", help="JSON validation report file")
    parser.add_argument("--output", help="Output markdown file")
    
    args = parser.parse_args()
    
    # Load validation data
    validation_path = Path(args.validation_file)
    if not validation_path.exists():
        print(f"❌ Validation file not found: {validation_path}")
        sys.exit(1)
        
    try:
        with open(validation_path, 'r') as f:
            validation_data = json.load(f)
    except json.JSONDecodeError as e:
        print(f"❌ Error parsing validation file: {e}")
        sys.exit(1)
    
    # Format report
    formatted_report = format_validation_report(validation_data)
    
    # Output report
    if args.output:
        with open(args.output, 'w') as f:
            f.write(formatted_report)
        print(f"📝 Formatted report saved to: {args.output}")
    else:
        print(formatted_report)

if __name__ == "__main__":
    main()