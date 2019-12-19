#pragma once

#include <stdint.h>

/// Parser buffer containing jpeg file into its segments.
class JPEGSegmentParser {
public:
    /// Describes one segment of the JPEG file.
	struct Segment {
		uint8_t kind;
		const char* payload_start;
		const char* payload_end;
	};

    /// Result of parsing operation.
	enum class ParseResult {
		Segment,
		EndOfFile,
		Error
	};

    /// Instantiate to parse the data in the range [begin, end).
	JPEGSegmentParser(const char* begin, const char* end);

    /// Tries to parse next segment. Will fill in the segment_out return
    /// parameter if successful, leaves it untouched otherwise.
	ParseResult next(Segment& segment_out);

private:
	enum class MarkerType {
		NoPayload,
		VariablePayload,
		Fixed2Payload,
		ScanPayload,
		Invalid
	};

	static MarkerType classifyMarker(uint8_t kind);

	const char* _current;
	const char* _end;
};
