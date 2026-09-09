// Keep test assertions active in Release builds.
#ifdef NDEBUG
#undef NDEBUG
#endif

#include "unicore.h"
#include <cassert>
#include <cstdio>

void test_empty()
{
	const char str[] = " ";

	UnicoreParser unicore_parser;

	for (unsigned i = 0; i < sizeof(str); ++i) {
		auto result = unicore_parser.parseChar(str[i]);
		assert(result == UnicoreParser::Result::None);
	}
}

void test_garbage()
{
	const char str[] = "#GARBAGE,BLA";

	UnicoreParser unicore_parser;

	for (unsigned i = 0; i < sizeof(str); ++i) {
		auto result = unicore_parser.parseChar(str[i]);
		assert(result == UnicoreParser::Result::None);
	}
}

void test_too_long()
{
	const char str[] =
		"#UNIHEADINGA,89,GPS,FINE,2251,168052600,0,0,18,11;SOL_COMPUTED,"
		"NARROW_INT,0.3718,67.0255,-0.7974,0.0000,0.8065,3.3818,\"999\","
		"31,21,21,18,3,01,3,f3,1234567890,1234567890,1234567890,1234567890,"
		"1234567890,1234567890,1234567890,1234567890,1234567890,1234567890,"
		"1234567890,1234567890,1234567890,1234567890,1234567890,1234567890,"
		"1234567890,1234567890,1234567890,1234567890,1234567890,1234567890,"
		"1234567890,1234567890,1234567890,1234567890,1234567890,1234567890,"
		"1234567890,1234567890,1234567890,1234567890,1234567890,1234567890,"
		"1234567890,1234567890,1234567890,1234567890,1234567890,1234567890,"
		"1234567890,1234567890,1234567890,1234567890,1234567890,1234567890,"
		"1234567890,1234567890,1234567890,1234567890,1234567890,1234567890,"
		"1234567890,ffffffff";

	UnicoreParser unicore_parser;

	for (unsigned i = 0; i < sizeof(str); ++i) {
		auto result = unicore_parser.parseChar(str[i]);
		assert(result == UnicoreParser::Result::None);
	}
}

void test_uniheadinga_wrong_crc()
{
	const char str[] =
		"#UNIHEADINGA,89,GPS,FINE,2251,168052600,0,0,18,11;SOL_COMPUTED,NARROW_INT,"
		"0.3718,67.0255,-0.7974,0.0000,0.8065,3.3818,\"999\",31,21,21,18,3,01,3,f3*"
		"ffffffff";

	UnicoreParser unicore_parser;

	for (unsigned i = 0; i < sizeof(str); ++i) {
		auto result = unicore_parser.parseChar(str[i]);

		if (result == UnicoreParser::Result::WrongCrc) {
			return;
		}
	}

	assert(false);
}


void test_uniheadinga()
{
	const char str[] =
		"#UNIHEADINGA,89,GPS,FINE,2251,168052600,0,0,18,11;SOL_COMPUTED,NARROW_INT,0.3718,67.0255,-0.7974,0.0000,0.8065,3.3818,\"999\",31,21,21,18,3,01,3,f3*ece5bb07";

	UnicoreParser unicore_parser;

	for (unsigned i = 0; i < sizeof(str); ++i) {
		auto result = unicore_parser.parseChar(str[i]);

		if (result == UnicoreParser::Result::GotHeading) {
			assert(unicore_parser.heading().heading_deg == 67.0255f);
			assert(unicore_parser.heading().baseline_m == 0.3718f);
			return;
		}
	}

	assert(false);
}

void test_uniheadinga_twice()
{
	const char str[] =
		"#UNIHEADINGA,89,GPS,FINE,2251,168052600,0,0,18,11;SOL_COMPUTED,NARROW_INT,0.3718,67.0255,-0.7974,0.0000,0.8065,3.3818,\"999\",31,21,21,18,3,01,3,f3*ece5bb07"
		"#UNIHEADINGA,88,GPS,FINE,2251,175391000,0,0,18,13;SOL_COMPUTED,NARROW_INT,0.3500,88.4714,-1.5969,0.0000,1.0175,2.8484,\"999\",28,18,18,12,3,01,3,f3*2888f737";

	UnicoreParser unicore_parser;

	unsigned num_parsed = 0;

	for (unsigned i = 0; i < sizeof(str); ++i) {
		auto result = unicore_parser.parseChar(str[i]);

		if (result == UnicoreParser::Result::GotHeading) {
			++num_parsed;
		}
	}

	assert(num_parsed == 2);
}

void test_agrica()
{
	const char str[] =
		"#AGRICA,68,GPS,FINE,2063,454587000,0,0,18,38;GNSS,236,19,7,26,6,16,9,4,4,12,10,"
		"9,306.7191,10724.0176,-16.4796,0.0089,0.0070,0.0181,67.9651,29.3584,0.0000,0.003,0.003,0.001,-0.002,0.021,"
		"0.039,0.025,40.07896719907,116.23652055432,67.3108,-2160482.7849,4383625.2350,4084735.7632,0.0140,0.0125,0.0296,0.0107,"
		"0.0198,0.0128,40.07627310896,116.11079363322,65.3740,0.00000000000,0.00000000000,0.0000,454587000,38.000,16.723207,-9.406086,0.000000,0.000000,8,0,0,0*e9402e02";

	UnicoreParser unicore_parser;

	for (unsigned i = 0; i < sizeof(str); ++i) {
		auto result = unicore_parser.parseChar(str[i]);

		if (result == UnicoreParser::Result::GotAgrica) {
			assert(unicore_parser.agrica().velocity_m_s == 0.003f);
			assert(unicore_parser.agrica().velocity_north_m_s == 0.003f);
			assert(unicore_parser.agrica().velocity_east_m_s == 0.001f);
			assert(unicore_parser.agrica().velocity_up_m_s == -0.002f);
			assert(unicore_parser.agrica().stddev_velocity_north_m_s == 0.021f);
			assert(unicore_parser.agrica().stddev_velocity_east_m_s == 0.039f);
			assert(unicore_parser.agrica().stddev_velocity_up_m_s == 0.025f);
			return;
		}
	}

	assert(false);
}

void test_unicore()
{
	test_empty();
	test_garbage();
	test_too_long();
	test_uniheadinga_wrong_crc();
	test_uniheadinga();
	test_uniheadinga_twice();
	test_agrica();
}

int main(int, char **)
{
	test_unicore();

	return 0;
}
