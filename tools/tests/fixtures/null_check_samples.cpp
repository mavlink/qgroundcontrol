// Test fixtures for vehicle_null_check.py analyzer
// Each section tests a specific pattern detection

// =============================================================================
// UNSAFE PATTERNS (should be detected)
// =============================================================================

// Pattern: unsafe_active_vehicle_direct
void unsafeDirectAccess() {
    // DETECT: Direct call without null check
    activeVehicle()->parameterManager()->getParameter(-1, "TEST");
}

// Pattern: unsafe_active_vehicle_use
void unsafeVariableUse() {
    Vehicle *vehicle = MultiVehicleManager::instance()->activeVehicle();
    // DETECT: Used without null check on next line
    vehicle->arm();
}

// Pattern: unsafe_get_parameter
void unsafeParameterAccess() {
    Vehicle *vehicle = getVehicle();
    if (vehicle) {
        // DETECT: getParameter result used directly
        int value = vehicle->parameterManager()->getParameter(-1, "PARAM")->rawValue().toInt();
    }
}

// =============================================================================
// SAFE PATTERNS (should NOT be detected)
// =============================================================================

// Safe: Null check with early return
void safeWithEarlyReturn() {
    Vehicle *vehicle = MultiVehicleManager::instance()->activeVehicle();
    if (!vehicle) {
        return;
    }
    vehicle->arm();  // OK - null check above
}

// Safe: Null check with nullptr comparison
void safeWithNullptrCheck() {
    Vehicle *vehicle = MultiVehicleManager::instance()->activeVehicle();
    if (vehicle == nullptr) {
        return;
    }
    vehicle->disarm();  // OK - null check above
}

// Safe: Null check with positive check
void safeWithPositiveCheck() {
    Vehicle *vehicle = MultiVehicleManager::instance()->activeVehicle();
    if (vehicle) {
        vehicle->land();  // OK - inside if block
    }
}

// Safe: Ternary operator check
void safeWithTernary() {
    Vehicle *vehicle = MultiVehicleManager::instance()->activeVehicle();
    QString name = vehicle ? vehicle->name() : "No vehicle";  // OK - ternary
}

// Safe: Parameter stored and checked
void safeParameterAccess() {
    Fact *param = vehicle->parameterManager()->getParameter(-1, "PARAM");
    if (!param) {
        qCWarning(Log) << "Parameter not found";
        return;
    }
    int value = param->rawValue().toInt();  // OK - null check above
}

// =============================================================================
// EDGE CASES
// =============================================================================

// Comment should be ignored
void commentedCode() {
    // activeVehicle()->method() - this is a comment, should not detect
    /* activeVehicle()->method() - block comment */
}

// Inside string literal (this file doesn't actually check strings, but good to note)
void stringLiteral() {
    QString s = "activeVehicle()->method()";  // Not real code
}
