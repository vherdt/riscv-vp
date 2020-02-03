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

constexpr unsigned SATP_MODE_BARE = 0;
constexpr unsigned SATP_MODE_SV32 = 1;
constexpr unsigned SATP_MODE_SV39 = 8;
constexpr unsigned SATP_MODE_SV48 = 9;
constexpr unsigned SATP_MODE_SV57 = 10;
constexpr unsigned SATP_MODE_SV64 = 11;