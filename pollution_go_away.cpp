#include <windows.h>

#include <cstddef>
#include <cstdint>

#include "xv2patcher.h"

namespace
{

// Packet 1 gets this participant value and resumes here in v1.26.
constexpr std::size_t PARTICIPANT_UPDATE_VALUE_RETURN = 0x478912;

typedef std::uint32_t (*ParticipantValueGetterType)(void *, std::int32_t *);

ParticipantValueGetterType participantValueGetterOriginal = nullptr;

} // namespace

extern "C"
{

PUBLIC void PollutionGoAwayParticipantValueSetup(ParticipantValueGetterType original)
{
	participantValueGetterOriginal = original;
}

PUBLIC std::uint32_t PollutionGoAwayParticipantValuePatched(
	void *context,
	std::int32_t *participantValue)
{
	const void *const caller = __builtin_return_address(0);
	const std::uint32_t result =
		participantValueGetterOriginal(context, participantValue);
	const std::uint8_t *const moduleBase =
		reinterpret_cast<const std::uint8_t *>(GetModuleHandle(nullptr));

	// Packet 1 copies this CRankManager value into the participant record. The
	// room handoff rejects -1 there, while a working client publishes 0. Limit
	// the fallback to this packet builder so other callers keep the native value.
	if (result != 0 && participantValue && *participantValue == -1 && moduleBase &&
		caller == moduleBase + PARTICIPANT_UPDATE_VALUE_RETURN)
	{
		*participantValue = 0;
	}

	return result;
}

} // extern "C"
