// ─── Preset Examples ────────────────────────────────────────────────────────

const PRESETS = {
  cf: `// Example 01: Constant Folding
// Expressions with constant operands are evaluated at compile time.
let x = 3 * 4 + 2;
let y = (10 - 5) * 2;
print(x);
print(y);
`,
  cp: `// Example 02: Constant Propagation
// Constants assigned to variables are propagated forward into uses.
let x = 10;
let y = x + 5;
let z = y * 2;
print(z);
`,
  dce: `// Example 03: Dead Code Elimination
// Overwritten variables and unused assignments are safely eliminated.
let x = 10;
x = 20;
let y = 99;
print(x);
`,
  combined: `// Example 04: Combined Optimization Pipeline
// Demonstrates folding -> propagation -> folding -> dead code elimination
let a = 2 * 3;
let b = a + 4;
let c = 100;
let d = b * 2;
print(d);
`,
  cond: `// Example 05: Conditionals & Branching
// Control flow TAC representation with labels and conditional jumps.
let x = 10;
let y = 20;
if (x < y) {
    let z = x + y;
    print(z);
} else {
    let w = 0;
    print(w);
}
`,
  err: `// Example 06: Compiler Error Diagnostics
// Syntax or semantic errors reported with exact line & column info.
let x = 10;
print(undeclared_var);
`
};

// ─── DOM References ─────────────────────────────────────────────────────────

const editor = document.getElementById("code-editor");
const lineNums = document.getElementById("editor-linenums");
const exampleSelect = document.getElementById("example-select");
const btnCompile = document.getElementById("btn-compile");
const btnExportJson = document.getElementById("btn-export-json");

const passCf = document.getElementById("pass-cf");
const passCp = document.getElementById("pass-cp");
const passDce = document.getElementById("pass-dce");

const metricStatus = document.getElementById("metric-status");
const metricInitCount = document.getElementById("metric-init-count");
const metricFinalCount = document.getElementById("metric-final-count");
const metricReduction = document.getElementById("metric-reduction");
const metricVerifier = document.getElementById("metric-verifier");

const stepCounter = document.getElementById("step-counter");
const stepperTrack = document.getElementById("stepper-track");

const irBeforeView = document.getElementById("ir-before-view");
const irAfterView = document.getElementById("ir-after-view");
const diffBeforeTitle = document.getElementById("diff-before-title");
const diffAfterTitle = document.getElementById("diff-after-title");

const logList = document.getElementById("log-list");
const logCount = document.getElementById("log-count");
const diagList = document.getElementById("diagnostics-list");
const diagCount = document.getElementById("diag-count");
const tokensContainer = document.getElementById("tokens-container");

const verifyBeforeOut = document.getElementById("verify-before-out");
const verifyAfterOut = document.getElementById("verify-after-out");
const verifySummary = document.getElementById("verify-summary");

let currentReport = null;
let currentStepIdx = 0;

// ─── Editor Line Numbers ────────────────────────────────────────────────────

function updateLineNumbers() {
  const lines = editor.value.split("\n").length;
  let nums = "";
  for (let i = 1; i <= Math.max(lines, 1); i++) {
    nums += i + "<br>";
  }
  lineNums.innerHTML = nums;
}

editor.addEventListener("input", updateLineNumbers);
editor.addEventListener("scroll", () => {
  lineNums.scrollTop = editor.scrollTop;
});

// Load default example
exampleSelect.addEventListener("change", (e) => {
  const key = e.target.value;
  if (PRESETS[key]) {
    editor.value = PRESETS[key];
    updateLineNumbers();
    compileAndOptimize();
  }
});

// ─── Tab Switching ──────────────────────────────────────────────────────────

document.querySelectorAll(".tab-btn").forEach((btn) => {
  btn.addEventListener("click", () => {
    document.querySelectorAll(".tab-btn").forEach((b) => b.classList.remove("active"));
    document.querySelectorAll(".tab-content").forEach((c) => c.classList.remove("active"));
    btn.classList.add("active");
    const tabId = btn.getAttribute("data-tab");
    document.getElementById(tabId).classList.add("active");
  });
});

// ─── IR Rendering Helpers ───────────────────────────────────────────────────

function renderIRList(irList, prevList = null) {
  if (!irList || irList.length === 0) {
    return `<div class="placeholder-msg">No IR instructions generated.</div>`;
  }

  let html = "";
  irList.forEach((instr, idx) => {
    let lineClass = "ir-line";
    let tagHtml = "";

    if (instr.eliminated) {
      lineClass += " eliminated";
      const newlyEliminated = !prevList || !prevList[idx] || !prevList[idx].eliminated;
      tagHtml = `<span class="line-tag tag-elim">${newlyEliminated ? "Removed here" : "Previously removed"}</span>`;
    } else if (prevList && prevList[idx]) {
      const prev = prevList[idx];
      if (prev.text !== instr.text) {
        if (instr.opcode === "CONST" && prev.opcode !== "CONST") {
          lineClass += " folded";
          tagHtml = `<span class="line-tag tag-mod">Folded</span>`;
        } else if (instr.arg1 !== prev.arg1 || instr.arg2 !== prev.arg2) {
          lineClass += " propagated";
          tagHtml = `<span class="line-tag tag-prop">Propagated</span>`;
        }
      }
    }

    html += `<div class="${lineClass}">
      <span><strong>#${instr.id}</strong> &nbsp; ${escapeHtml(instr.text)}</span>
      ${tagHtml}
    </div>`;
  });
  return html;
}

function escapeHtml(str) {
  if (!str) return "";
  return str.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

// ─── Step Navigation ────────────────────────────────────────────────────────

function selectStep(stepIdx) {
  if (!currentReport) return;
  currentStepIdx = stepIdx;

  // Update active state in stepper nodes
  const nodes = stepperTrack.querySelectorAll(".stepper-node");
  nodes.forEach((n, idx) => {
    if (idx === stepIdx) n.classList.add("active");
    else n.classList.remove("active");
  });

  const totalSteps = currentReport.passes.length + 1;
  stepCounter.textContent = `Step ${stepIdx} of ${totalSteps - 1}`;

  if (stepIdx === 0) {
    // Initial IR view
    diffBeforeTitle.textContent = "Initial TAC IR";
    diffAfterTitle.textContent = "Initial TAC IR";
    irBeforeView.innerHTML = renderIRList(currentReport.initial_ir);
    irAfterView.innerHTML = renderIRList(currentReport.initial_ir);
    renderExplanations(["Initial Three-Address Code (TAC) generated from AST."]);
  } else {
    const snap = currentReport.passes[stepIdx - 1];
    const prevIR = (stepIdx - 1 === 0) ? currentReport.initial_ir : currentReport.passes[stepIdx - 2].ir_after;

    diffBeforeTitle.textContent = `IR Before ${snap.name} (Round ${snap.iteration})`;
    diffAfterTitle.textContent = `IR After ${snap.name} (Round ${snap.iteration})`;

    irBeforeView.innerHTML = renderIRList(prevIR);
    irAfterView.innerHTML = renderIRList(snap.ir_after, prevIR);
    renderExplanations(snap.explanations);
  }
}

function renderExplanations(explanations) {
  if (!explanations || explanations.length === 0) {
    logList.innerHTML = `<div class="empty-log">Pass made no modifications during this iteration (fixed point convergence).</div>`;
    logCount.textContent = "0";
    return;
  }

  logCount.textContent = explanations.length;
  let html = "";
  explanations.forEach((exp) => {
    const isElim = exp.includes("Eliminated");
    html += `<div class="log-item ${isElim ? "elim" : ""}">
      <span class="log-bullet">${isElim ? "✗" : "✓"}</span>
      <span>${escapeHtml(exp)}</span>
    </div>`;
  });
  logList.innerHTML = html;
}

// ─── Pipeline Population ────────────────────────────────────────────────────

function buildPipeline(report) {
  stepperTrack.innerHTML = "";

  // Step 0: Initial IR
  const initNode = document.createElement("div");
  initNode.className = "stepper-node active";
  initNode.textContent = "0. Initial IR";
  initNode.addEventListener("click", () => selectStep(0));
  stepperTrack.appendChild(initNode);

  // Subsequent passes
  report.passes.forEach((snap, idx) => {
    const node = document.createElement("div");
    node.className = "stepper-node" + (snap.changed ? " has-changes" : "");
    node.textContent = `${idx + 1}. ${snap.name} (R${snap.iteration})`;
    node.addEventListener("click", () => selectStep(idx + 1));
    stepperTrack.appendChild(node);
  });

  // Select last pass or initial
  const defaultIdx = report.passes.length > 0 ? report.passes.length : 0;
  selectStep(defaultIdx);
}

// ─── Verification & Diagnostics ─────────────────────────────────────────────

function updateVerification(report) {
  const beforeOut = (report.execution_before && report.execution_before.output) || [];
  const afterOut = (report.execution_after && report.execution_after.output) || [];

  verifyBeforeOut.textContent = beforeOut.length ? beforeOut.join("\n") : "// (No print outputs)";
  verifyAfterOut.textContent = afterOut.length ? afterOut.join("\n") : "// (No print outputs)";

  if (!report.success) {
    verifySummary.innerHTML = `<strong>Compilation or execution failed</strong>`;
    verifySummary.style.color = "var(--accent-red)";
    metricVerifier.textContent = "FAILED ❌";
    metricVerifier.style.color = "var(--accent-red)";
  } else if (report.results_match) {
    verifySummary.innerHTML = `Semantic Equivalence: <strong>PASS ✅ (Output before == Output after)</strong>`;
    verifySummary.style.color = "var(--accent-green)";
    metricVerifier.textContent = "VERIFIED ✅";
    metricVerifier.style.color = "var(--accent-green)";
  } else {
    verifySummary.innerHTML = `Semantic Equivalence: <strong>FAIL ❌ (Output discrepancy detected)</strong>`;
    verifySummary.style.color = "var(--accent-red)";
    metricVerifier.textContent = "MISMATCH ❌";
    metricVerifier.style.color = "var(--accent-red)";
  }

  function resetReportView() {
    currentReport = null;
    currentStepIdx = 0;
    metricInitCount.textContent = "-";
    metricFinalCount.textContent = "-";
    metricReduction.textContent = "-";
    metricVerifier.textContent = "-";
    stepCounter.textContent = "Step 0 of 0";
    stepperTrack.innerHTML = `<div class="stepper-node active">Initial IR</div>`;
    irBeforeView.innerHTML = `<div class="placeholder-msg">No IR available.</div>`;
    irAfterView.innerHTML = `<div class="placeholder-msg">No IR available.</div>`;
    diffBeforeTitle.textContent = "Initial TAC IR";
    diffAfterTitle.textContent = "Pass Transformed IR";
    logList.innerHTML = `<div class="empty-log">No transformations applied.</div>`;
    logCount.textContent = "0";
    verifyBeforeOut.textContent = "// No execution";
    verifyAfterOut.textContent = "// No execution";
    verifySummary.innerHTML = "Semantic Equivalence: <strong>Pending</strong>";
  }
}

function updateTokens(tokens) {
  if (!tokens || tokens.length === 0) {
    tokensContainer.innerHTML = `<span class="empty-log">No tokens.</span>`;
    return;
  }
  let html = "";
  tokens.forEach((t) => {
    html += `<div class="token-pill">
      <span class="tok-type">${escapeHtml(t.type)}</span>
      <span class="tok-val">${escapeHtml(t.value || "EOF")}</span>
    </div>`;
  });
  tokensContainer.innerHTML = html;
}

function updateDiagnostics(errors) {
  if (!errors || errors.length === 0) {
    diagList.innerHTML = `<div class="empty-log">No compiler errors or warnings reported.</div>`;
    diagCount.textContent = "0";
    return;
  }
  diagCount.textContent = errors.length;
  let html = "";
  errors.forEach((err) => {
    html += `<div class="diag-item">
      <strong>[${escapeHtml(err.phase).toUpperCase()}]</strong> Line ${err.line}, Col ${err.column}: ${escapeHtml(err.message)}
    </div>`;
  });
  diagList.innerHTML = html;
}

// ─── Compile & Optimize Action ──────────────────────────────────────────────

async function compileAndOptimize() {
  const source = editor.value;
  btnCompile.disabled = true;
  btnCompile.innerHTML = "Optimizing...";

  try {
    const payload = {
      source: source,
      enable_cf: passCf.checked,
      enable_cp: passCp.checked,
      enable_dce: passDce.checked,
      max_iterations: 10
    };

    const res = await fetch("/api/compile", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });

    const data = await res.json();
    currentReport = data;

    if (!data.success) {
      metricStatus.textContent = "Error";
      metricStatus.className = "metric-value text-red";
      metricStatus.style.color = "var(--accent-red)";
      updateDiagnostics(data.errors);
      updateTokens(data.tokens);
      irBeforeView.innerHTML = renderIRList(data.initial_ir);
      irAfterView.innerHTML = `<div class="placeholder-msg" style="color:var(--accent-red)">Compilation halted due to diagnostics.</div>`;
      updateVerification(data);
      stepperTrack.innerHTML = `<div class="stepper-node active">Compilation failed</div>`;
      stepCounter.textContent = "Step 0 of 0";
      // Switch to diagnostics tab
      document.querySelector('[data-tab="tab-diagnostics"]').click();
      return;
    }

    // Success metrics
    metricStatus.textContent = "Optimized";
    metricStatus.className = "metric-value status-success";
    metricStatus.style.color = "var(--accent-green)";

    const initActive = data.initial_ir.filter((i) => !i.eliminated).length;
    const finalActive = data.final_ir.filter((i) => !i.eliminated).length;
    metricInitCount.textContent = initActive;
    metricFinalCount.textContent = finalActive;

    const diff = initActive - finalActive;
    const pct = initActive > 0 ? Math.round((diff / initActive) * 100) : 0;
    metricReduction.textContent = diff > 0 ? `${pct}% fewer (${diff} removed)` : "0%";

    buildPipeline(data);
    updateVerification(data);
    updateTokens(data.tokens);
    updateDiagnostics([]);

  } catch (err) {
    console.error("Compilation error:", err);
    resetReportView();
    metricStatus.textContent = "Network Error";
    metricStatus.style.color = "var(--accent-red)";
  } finally {
    btnCompile.disabled = false;
    btnCompile.innerHTML = "Compile &amp; Optimize";
  }
}

btnCompile.addEventListener("click", compileAndOptimize);

// ─── Export JSON ────────────────────────────────────────────────────────────

btnExportJson.addEventListener("click", () => {
  if (!currentReport) {
    alert("Please compile and optimize a program first.");
    return;
  }
  const blob = new Blob([JSON.stringify(currentReport, null, 2)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = "optimization_report.json";
  a.click();
  URL.revokeObjectURL(url);
});

// Initialize on page load
window.addEventListener("DOMContentLoaded", () => {
  editor.value = PRESETS.combined;
  updateLineNumbers();
  compileAndOptimize();
});
