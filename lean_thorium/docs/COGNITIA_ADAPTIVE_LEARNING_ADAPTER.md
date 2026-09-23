# Lean Thorium - Cognitia Adaptive Learning Adapter

## 1. Overview & Adapter Role

Lean Thorium integrates with Cognitia as an **observation and content provider**, consuming advisory machine learning predictions through the Cognitia Runtime Gateway.

Lean Thorium is strictly a **consumer** of Cognitia intelligence and does NOT own or implement machine learning semantics, model registries, or learning databases.

---

## 2. Request & Response Architecture

```text
Lean Thorium Browser
        │
        │ 1. Browser Observation / Telemetry
        ▼
Cognitia Adapter (Client)
        │
        │ 2. POST /v1/learning/predict
        ▼
Cognitia Standalone Runtime
        │
        ├── Representation Boundary (Sanitization & Threat Redaction)
        ├── Laya Provider (Typed Decision Inference)
        └── File Persistence P1 (Durable Journaling)
        │
        │ 3. AdaptiveLearningResult (authority = "NONE")
        ▼
Lean Thorium Adapter
        │
        │ 4. Advisory consumption (Zero Actuator/Execution Authority)
        ▼
Lean Thorium
```

---

## 3. Host Application Invariants

- **No AI / ML Runtimes in Thorium**: No PyTorch, TensorFlow, scikit-learn, ONNX, or model weight files inside the browser codebase.
- **No Duplicate Persistence**: Thorium does not maintain `learning.db`, `model_registry.json`, or local model state files.
- **Untrusted Web Boundary**: All web content extracted from browser frames is treated as untrusted data. Prompt injection attempts and script execution directives are neutralized before model consumption.
- **Zero Production Authority**: All predictions received from Cognitia carry `authority = "NONE"` and remain advisory.
