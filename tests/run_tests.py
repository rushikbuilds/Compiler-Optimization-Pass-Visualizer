#!/usr/bin/env python3
import json
import os
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
COMPILER_BIN = os.path.join(PROJECT_ROOT, "compiler.exe")

def run_compiler(args=None, stdin_text=None):
    cmd = [COMPILER_BIN] + (args or [])
    res = subprocess.run(cmd, input=stdin_text, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=PROJECT_ROOT)
    return res

def test_constant_folding():
    print("[TEST] Running Constant Folding Verification...")
    src = "let x = 3 * 4 + 2;\nprint(x);\n"
    res = run_compiler(stdin_text=src)
    assert res.returncode == 0, f"Process failed: {res.stderr}"
    data = json.loads(res.stdout)
    assert data["success"] is True
    assert data["results_match"] is True
    assert data["execution_before"]["output"] == ["14"]
    assert data["execution_after"]["output"] == ["14"]
    # Check that folding occurred
    cf_pass = [p for p in data["passes"] if p["name"] == "Constant Folding" and p["changed"]]
    assert len(cf_pass) > 0, "Constant folding should have changed IR"
    print("  -> Passed!")

def test_constant_propagation():
    print("[TEST] Running Constant Propagation Verification...")
    src = "let x = 10;\nlet y = x + 5;\nprint(y);\n"
    res = run_compiler(stdin_text=src)
    assert res.returncode == 0
    data = json.loads(res.stdout)
    assert data["success"] is True
    assert data["results_match"] is True
    assert data["execution_after"]["output"] == ["15"]
    cp_pass = [p for p in data["passes"] if p["name"] == "Constant Propagation" and p["changed"]]
    assert len(cp_pass) > 0, "Constant propagation should have changed IR"
    print("  -> Passed!")

def test_dead_code_elimination():
    print("[TEST] Running Dead Code Elimination Verification...")
    src = "let x = 10;\nx = 20;\nlet y = 99;\nprint(x);\n"
    res = run_compiler(stdin_text=src)
    assert res.returncode == 0
    data = json.loads(res.stdout)
    assert data["success"] is True
    assert data["results_match"] is True
    assert data["execution_after"]["output"] == ["20"]
    dce_pass = [p for p in data["passes"] if p["name"] == "Dead Code Elimination" and p["changed"]]
    assert len(dce_pass) > 0, "DCE should have changed IR"
    # y = 99 should be eliminated
    eliminated_dests = [i["dest"] for i in data["final_ir"] if i["eliminated"]]
    assert "y" in eliminated_dests, "y should be eliminated by DCE"
    print("  -> Passed!")

def test_combined_fixed_point():
    print("[TEST] Running Combined Fixed-Point Pipeline Verification...")
    src = "let a = 2 * 3;\nlet b = a + 4;\nlet c = 100;\nlet d = b * 2;\nprint(d);\n"
    res = run_compiler(stdin_text=src)
    assert res.returncode == 0
    data = json.loads(res.stdout)
    assert data["success"] is True
    assert data["results_match"] is True
    assert data["execution_after"]["output"] == ["20"]
    print("  -> Passed!")

def test_error_diagnostics():
    print("[TEST] Running Error Diagnostics Verification...")
    src = "let a = 10;\nprint(b);\n"
    res = run_compiler(stdin_text=src)
    assert res.returncode == 0
    data = json.loads(res.stdout)
    assert data["success"] is False
    assert len(data["errors"]) > 0
    assert "undeclared variable 'b'" in data["errors"][0]["message"]
    print("  -> Passed!")

def test_branch_dce_preserves_output():
    print("[TEST] Running Branch DCE Safety Verification...")
    src = "let x = 0;\nif (1) { x = 1; } else { x = 2; }\nprint(x);\n"
    res = run_compiler(stdin_text=src)
    assert res.returncode == 0
    data = json.loads(res.stdout)
    assert data["success"] is True
    assert data["results_match"] is True
    assert data["execution_before"]["output"] == ["1"]
    assert data["execution_after"]["output"] == ["1"]
    print("  -> Passed!")

def test_dce_preserves_runtime_faults():
    print("[TEST] Running Runtime Fault Preservation Verification...")
    src = "let unused = 1 / 0;\nprint(7);\n"
    res = run_compiler(stdin_text=src)
    assert res.returncode == 0
    data = json.loads(res.stdout)
    assert data["success"] is False
    assert data["execution_before"]["success"] is False
    assert data["execution_after"]["success"] is False
    assert data["results_match"] is True
    print("  -> Passed!")

def test_integer_range_diagnostics():
    print("[TEST] Running Integer Range Verification...")
    src = "let x = 2147483648;\nprint(x);\n"
    res = run_compiler(stdin_text=src)
    assert res.returncode == 0
    data = json.loads(res.stdout)
    assert data["success"] is False
    assert any("out of range" in e["message"] for e in data["errors"])
    print("  -> Passed!")

def test_cli_argument_diagnostics():
    print("[TEST] Running CLI Argument Verification...")
    res = run_compiler(["--max-iterations", "invalid"], "print(1);\n")
    assert res.returncode != 0
    data = json.loads(res.stdout)
    assert data["success"] is False
    assert data["errors"][0]["phase"] == "cli"
    print("  -> Passed!")

def test_scope_and_definite_assignment():
    print("[TEST] Running Scope and Definite Assignment Verification...")
    res = run_compiler(stdin_text="if (0) { let x = 1; }\nprint(x);\n")
    assert res.returncode == 0
    data = json.loads(res.stdout)
    assert data["success"] is False
    assert any("undeclared variable 'x'" in e["message"] for e in data["errors"])
    print("  -> Passed!")

def test_redeclaration_diagnostics():
    print("[TEST] Running Redeclaration Verification...")
    res = run_compiler(stdin_text="let x = 1;\nlet x = 2;\nprint(x);\n")
    assert res.returncode == 0
    data = json.loads(res.stdout)
    assert data["success"] is False
    assert any("Redeclaration" in e["message"] for e in data["errors"])
    print("  -> Passed!")

def main():
    if not os.path.exists(COMPILER_BIN):
        print(f"Error: {COMPILER_BIN} not found. Build the project first.")
        sys.exit(1)

    print("========================================")
    print("Compiler Optimization Test Suite Runner")
    print("========================================")
    tests = [
        test_constant_folding,
        test_constant_propagation,
        test_dead_code_elimination,
        test_combined_fixed_point,
        test_error_diagnostics,
        test_branch_dce_preserves_output,
        test_dce_preserves_runtime_faults,
        test_integer_range_diagnostics,
        test_cli_argument_diagnostics,
        test_scope_and_definite_assignment,
        test_redeclaration_diagnostics,
    ]

    failed = 0
    for t in tests:
        try:
            t()
        except Exception as e:
            print(f"  -> FAILED: {e}")
            failed += 1

    print("========================================")
    if failed == 0:
        print(f"ALL TESTS PASSED! ({len(tests)}/{len(tests)})")
        sys.exit(0)
    else:
        print(f"{failed} tests failed.")
        sys.exit(1)

if __name__ == "__main__":
    main()
