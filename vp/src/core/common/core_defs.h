#pragma once

enum Architecture {
	RV32 = 1,
	RV64 = 2,
	RV128 = 3,
};

enum class CoreExecStatus {
	Runnable,
	HitBreakpoint,
	Terminated,
};
