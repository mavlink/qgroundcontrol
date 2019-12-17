#include "JPEGSegmentParser.h"

JPEGSegmentParser::JPEGSegmentParser(const char* begin, const char* end)
	: _current(begin), _end(end)
{
}

JPEGSegmentParser::ParseResult JPEGSegmentParser::next(Segment& segment_out)
{
	if (_current == _end) {
		return ParseResult::EndOfFile;
	}
	if (static_cast<uint8_t>(*_current) != 0xff) {
		return ParseResult::Error;
	}
	++_current;

	if (_current == _end || (static_cast<uint8_t>(*_current) & 0xc0) != 0xc0) {
		return ParseResult::Error;
	}
	uint8_t kind = *_current;
	++_current;
	MarkerType marker_class = classifyMarker(kind);
	switch (marker_class) {
		case MarkerType::NoPayload: {
			segment_out.kind = kind;
			segment_out.payload_start = _current;
			segment_out.payload_end = _current;
			if (kind == 0xd9) {
				// On EOI, skip remainder of file.
				_current = _end;
			}
			return ParseResult::Segment;
		}
		case MarkerType::Fixed2Payload: {
			if (_end - _current < 2) {
				return ParseResult::Error;
			}
			segment_out.kind = kind;
			segment_out.payload_start = _current;
			segment_out.payload_end = _current + 2;
			_current += 2;
			return ParseResult::Segment;
		}
		case MarkerType::VariablePayload: {
			if (_end - _current < 2) {
				return ParseResult::Error;
			}
			uint8_t hi = static_cast<uint8_t>(_current[0]);
			uint8_t lo = static_cast<uint8_t>(_current[1]);
			uint32_t size = lo + (hi << 8);
			if (_end - _current < size) {
				return ParseResult::Error;
			}
			segment_out.kind = kind;
			segment_out.payload_start = _current + 2;
			segment_out.payload_end = _current + size;
			_current += size;
			return ParseResult::Segment;
		}
		case MarkerType::ScanPayload: {
			const char* payload_start = _current;
			for (;;) {
				if (_end - _current < 2) {
					return ParseResult::Error;
				}
				if (static_cast<uint8_t>(_current[0]) != 0xff) {
					++_current;
					continue;
				}
				uint8_t next = static_cast<uint8_t>(_current[1]);
				if (next == 0x00 || next == 0xff) {
					_current += 2;
					continue;
				}
				break;
			}
			segment_out.kind = kind;
			segment_out.payload_start = payload_start;
			segment_out.payload_end = _current;
			return ParseResult::Segment;
		}
		default: {
			return ParseResult::Error;
		}
	}
}

JPEGSegmentParser::MarkerType JPEGSegmentParser::classifyMarker(uint8_t kind)
{
	if ((kind >= 0xd0) && (kind <= 0xd9)) {
		// 0xd0..0xd7: RST0..RST7, 0xd8: SOI, 0xd9: EOI
		return MarkerType::NoPayload;
	} else if (kind == 0xdd) {
		return MarkerType::Fixed2Payload;
	} else if (kind == 0xda) {
		return MarkerType::ScanPayload;
	} else if (kind != 0xff) {
		return MarkerType::VariablePayload;
	} else {
		return MarkerType::Invalid;
	}
}
