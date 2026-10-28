#include <windows.h>

#include <cstddef>
#include <cstdint>

#include "xv2patcher.h"
#include "debug.h"

namespace
{

	// Packet 1 gets this participant value and resumes here
	const void *PARTICIPANT_UPDATE_VALUE_RETURN = nullptr;

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

		// Check whether this function was called from the Packet 1 builder.
		// The located address is already absolute, so no module base is added.
		const bool isParticipantUpdatePacket =
			PARTICIPANT_UPDATE_VALUE_RETURN != nullptr &&
			caller == PARTICIPANT_UPDATE_VALUE_RETURN;

		// Log the value Packet 1 received before applying any correction.
		if (isParticipantUpdatePacket)
		{
			if (participantValue)
			{
				DPRINTF(
					"Participant update: result=%u, value=%d\n",
					result,
					*participantValue);
			}
			else
			{
				// Avoid dereferencing the pointer if Packet 1 supplied null.
				DPRINTF(
					"Participant update: result=%u, value=null\n",
					result);
			}
		}

		// Packet 1 copies this CRankManager value into the participant record. The
		// room handoff rejects -1 there, while a working client publishes 0. Limit
		// the fallback to this packet builder so other callers keep the native value.
		if (result != 0 &&
			participantValue &&
			*participantValue == -1 &&
			isParticipantUpdatePacket)
		{
			// Replace the invalid participant value with the working fallback.
			*participantValue = 0;

			// Confirm that the correction was actually applied.
			DPRINTF(
				"Participant update: corrected value from -1 to 0.\n");
		}

		return result;
	}

	PUBLIC void OnLocateParticipateUpdateValueReturn(
		void *addr,
		std::size_t)
	{
		// Save the absolute address located by the patcher.
		// The unused size argument is retained to match the callback signature.
		PARTICIPANT_UPDATE_VALUE_RETURN = addr;

		// Log the address so it can be compared with the runtime caller address.
		DPRINTF(
			"Participant update return located at %p.\n",
			addr);
	}
} // extern "C"
