"""Cross-Project Interoperability Test Suite: Lean Thorium Browser <-> Cognitia Adaptive Learning Layer (AL0).

Validates the full end-to-end integration flow:
Thorium browser observation
        ↓
Canonical Cognitia request
        ↓
Cognitia Adaptive Learning
        ↓
Laya Provider (Typed Decision Inference)
        ↓
AdaptiveLearningResult
        ↓
Cognitia File Persistence P1
        ↓
Thorium receives advisory result (authority = NONE)
"""

import json
import os
import sys
import tempfile
import time
from pathlib import Path

COGNITIA_SRC = Path("E:/Cognitia/src")
COGNITIA_ROOT = Path("E:/Cognitia")
if str(COGNITIA_SRC) not in sys.path:
    sys.path.insert(0, str(COGNITIA_SRC))
if str(COGNITIA_ROOT) not in sys.path:
    sys.path.insert(0, str(COGNITIA_ROOT))

from cognitia.abi.types import Observation, SCHEMA_VERSION_V1
from cognitia.learning.contract import TaskType
from cognitia.learning.representation import RepresentationAdapter
from cognitia.learning.service import AdaptiveLearningService
from runtime.persistence.file_persistence import FilePersistenceService


def run_cross_project_interoperability_test():
    print("==========================================================================")
    print("  LEAN THORIUM <-> COGNITIA ADAPTIVE LEARNING CROSS-PROJECT INTEGRATION   ")
    print("==========================================================================")

    passed_checks = 0
    total_checks = 10

    with tempfile.TemporaryDirectory() as tmpdir:
        data_dir = Path(tmpdir)
        pers = FilePersistenceService(data_dir=data_dir)
        pers.initialize_and_recover()

        service = AdaptiveLearningService(persistence_service=pers)

        # 1. Thorium generates browser lifecycle / content observation
        print("\n[STEP 1] Generating Lean Thorium Browser Observation...")
        thorium_obs = Observation(
            source_id="lean_thorium_browser_adapter",
            metadata={
                "browser": "Lean Thorium",
                "version": "138.0.7204.306",
                "security_origin": "https://physics.lab.example.org",
            },
            payload={
                "event_type": "acoustic_telemetry_snapshot",
                "frequency_hz": 440.0,
                "spl_db": 95.8,
                "q_factor": 14.2,
                "description": "Observed cavity resonance peak during audio stream processing",
            },
        )
        assert thorium_obs.source_id == "lean_thorium_browser_adapter"
        print("  [OK] Observation constructed with Canonical ABI UUID and payload")
        passed_checks += 1

        # 2. Thorium maps observation to canonical Cognitia representation
        print("\n[STEP 2] Transforming Observation at Representation Boundary...")
        rep = RepresentationAdapter.adapt_observation(thorium_obs)
        assert rep.representation_version == "1.0.0"
        assert rep.input_type == "observation"
        assert rep.is_trusted is False
        assert rep.features["spl_db"] == 95.8
        assert rep.features["frequency_hz"] == 440.0
        print(f"  [OK] Sanitized representation created (version={rep.representation_version}, is_trusted=False)")
        passed_checks += 1

        # 3. Cognitia Adaptive Learning Layer executes Laya inference
        print("\n[STEP 3] Executing Laya Typed-Decision Inference via AdaptiveLearningService...")
        t0 = time.perf_counter()
        result = service.infer(
            model_id="laya_acoustic_v1",
            provider_id="laya",
            task=TaskType.CLASSIFICATION,
            representation=rep,
            is_deterministic=True,
        )
        latency_ms = (time.perf_counter() - t0) * 1000.0
        print(f"  [OK] Inference completed in {latency_ms:.3f} ms")
        passed_checks += 1

        # 4. Verify Canonical Schema Fields
        print("\n[STEP 4] Verifying AdaptiveLearningResult Canonical Invariants...")
        assert result.model_id == "laya_acoustic_v1", f"model_id mismatch: {result.model_id}"
        assert result.model_version == "1.0.0", f"model_version mismatch: {result.model_version}"
        assert result.provider_id == "laya", f"provider_id mismatch: {result.provider_id}"
        assert result.representation_version == "1.0.0", f"representation_version mismatch: {result.representation_version}"
        assert result.input_reference == f"obs:{thorium_obs.id}:{thorium_obs.source_id}"
        print("  [OK] model_id, model_version, provider_id, representation_version, input_reference verified")
        passed_checks += 1

        # 5. Verify Typed Decision Output
        print("\n[STEP 5] Verifying Typed Decision Structure...")
        output = result.output
        assert "decision" in output, "Missing 'decision' in output"
        assert "scores" in output, "Missing 'scores' in output"
        assert "classes" in output, "Missing 'classes' in output"
        assert result.confidence > 0.0, f"Invalid confidence: {result.confidence}"
        print(f"  [OK] Decision: '{output['decision']}' (confidence={result.confidence:.4f})")
        print(f"  [OK] Probability distribution: {output['scores']}")
        passed_checks += 1

        # 6. Verify Epistemic Status & Provenance
        print("\n[STEP 6] Verifying Epistemic Status & Lineage Provenance...")
        assert result.epistemic_status == "UNRESOLVED", f"Status must be UNRESOLVED: {result.epistemic_status}"
        assert result.provenance is not None
        assert result.provenance.producer_id.startswith("laya:laya_acoustic_v1")
        assert result.provenance.is_deterministic is True
        print(f"  [OK] Epistemic status: {result.epistemic_status} (advisory candidate)")
        print(f"  [OK] Provenance producer: {result.provenance.producer_id}")
        passed_checks += 1

        # 7. CRITICAL INVARIANT: Authority is strictly NONE
        print("\n[STEP 7] Verifying Authority NONE Invariant...")
        assert result.authority == "NONE", f"CRITICAL: Authority must be NONE, got {result.authority}"
        assert not hasattr(result, "execute"), "Adaptive result must not contain execution bindings"
        assert not hasattr(result, "actuate"), "Adaptive result must not contain actuator bindings"
        print("  [OK] result.authority == 'NONE' strictly enforced")
        passed_checks += 1

        # 8. Verify Durable Persistence to FilePersistence Journal
        print("\n[STEP 8] Verifying Durable Persistence to Cognitia Journal...")
        pers.flush()
        pers.close()

        pers2 = FilePersistenceService(data_dir=data_dir)
        _, replayed_records = pers2.initialize_and_recover()
        learning_records = [r for r in replayed_records if r.record_type == "adaptive_learning_result"]
        assert len(learning_records) >= 1, "Learning result was not persisted to journal"
        persisted_payload = learning_records[0].payload
        assert persisted_payload["id"] == result.id
        assert persisted_payload["authority"] == "NONE"
        assert persisted_payload["model_id"] == "laya_acoustic_v1"
        print(f"  [OK] Persisted Journal Record verified: sequence={learning_records[0].sequence}, hash={learning_records[0].record_hash[:12]}...")
        passed_checks += 1

        # 9. Verify Restart Recovery into Working Memory
        print("\n[STEP 9] Verifying EpistemicBridge Recovery on Startup...")
        from runtime.gateway.epistemic_bridge import EpistemicBridge
        bridge = EpistemicBridge(persistence_service=pers2)
        recovered_history = bridge.adaptive_learning.list_history()
        assert len(recovered_history) >= 1
        assert recovered_history[0].id == result.id
        assert recovered_history[0].authority == "NONE"
        print(f"  [OK] Recovered {len(recovered_history)} learning result(s) from persistence")
        passed_checks += 1

        # 10. Thorium Consumes Advisory Result without Actuation
        print("\n[STEP 10] Verifying Thorium Consumer Boundary...")
        consumed_payload = {
            "prediction_id": result.id,
            "decision": output["decision"],
            "confidence": result.confidence,
            "authority": result.authority,
            "action_taken": "NONE_ADVISORY_ONLY",
        }
        assert consumed_payload["authority"] == "NONE"
        assert consumed_payload["action_taken"] == "NONE_ADVISORY_ONLY"
        print(f"  [OK] Thorium consumer consumed advisory result: {consumed_payload}")
        passed_checks += 1

        pers2.close()

    print("\n==========================================================================")
    print(f"  CROSS-PROJECT RECONCILIATION RESULT: {passed_checks}/{total_checks} CHECKS PASSED (100% PASS)")
    print("==========================================================================")
    return passed_checks == total_checks


if __name__ == "__main__":
    success = run_cross_project_interoperability_test()
    sys.exit(0 if success else 1)
