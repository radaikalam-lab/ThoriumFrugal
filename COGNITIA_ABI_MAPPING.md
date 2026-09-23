# Cognitia Cognitive ABI v1.0.0 — Field Mapping & Serialization Specification

**Target Version:** Cognitia ABI `1.0.0` (`E:\Cognitia\src\cognitia\abi\types.py`)  
**Adapter Implementation:** `E:\Thorium\lean_thorium\src\chrome\browser\cognitia_adapter\protocol\`

---

## 1. Field-by-Field Canonical Mapping

| ABI Top-Level Field | Data Type | Requirement | Thorium C++ Source | Canonical ABI Mapping (`CognitiveObject`) |
| :--- | :--- | :--- | :--- | :--- |
| `id` | `string` (UUIDv4) | **Mandatory** | `base::UnguessableToken::Create().ToString()` | `CognitiveObject.id` |
| `schema_version` | `string` (SemVer) | **Mandatory** | `"1.0.0"` | `CognitiveObject.schema_version` |
| `created_at` | `string` (ISO-8601 UTC) | **Mandatory** | `exploded.year-month-day...Z` | `CognitiveObject.created_at` |
| `source_id` | `string` | **Mandatory** | `"thorium_browser_adapter"` | `Observation.source_id` |
| `metadata` | `object` | Optional | `{ "transport": "named_pipe" }` | `CognitiveObject.metadata` |
| `payload` | `object` | **Mandatory** | Nested browser/page/event dictionary | `Observation.payload` |

---

## 2. Deterministic Serialization Guarantees

As specified in `E:\Cognitia\contracts\cognitive-abi.md`:
1. **Key Ordering:** Lexicographical ascending sort order.
2. **Encodings:** UTF-8 canonical JSON without byte-order marks.
3. **Whitespace:** Compact separators (`,` and `:` with no extraneous padding).
4. **Float / Metric Precision:** Deterministic standard notation without `NaN` or `Infinity`.
