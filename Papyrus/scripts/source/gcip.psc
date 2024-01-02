Scriptname GCIP hidden

; Encounter Planner
bool Function tryLock(Form form, String lock_id, String action) global native
bool Function tryInterceptLock(Form form, String lock_id, String action) global native
bool Function shareLock(Form form, String lock_id, String allowed_lock_id) global native
bool Function isLocked(Form form, String lock_id) global native
bool Function unlock(Form form, String lock_id) global native
String Function getCurrentAction(Form form) global native