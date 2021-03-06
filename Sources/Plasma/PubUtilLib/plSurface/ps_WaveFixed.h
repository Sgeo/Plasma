


static const uint32_t ps_WaveFixedbyteLen = 108;

static const uint8_t ps_WaveFixedCodes[] = {
	0x1,	0x1,	0xff,	0xff,
	0x51,	0x0,	0x0,	0x0,
	0x0,	0x0,	0xf,	0xa0,
	0x0,	0x0,	0x80,	0x3f,
	0x0,	0x0,	0x0,	0x0,
	0x0,	0x0,	0x0,	0x0,
	0x0,	0x0,	0x80,	0x3f,
	0x42,	0x0,	0x0,	0x0,
	0x0,	0x0,	0xf,	0xb0,
	0x49,	0x0,	0x0,	0x0,
	0x1,	0x0,	0xf,	0xb0,
	0x0,	0x0,	0xe4,	0xb4,
	0x49,	0x0,	0x0,	0x0,
	0x2,	0x0,	0xf,	0xb0,
	0x0,	0x0,	0xe4,	0xb4,
	0x4d,	0x0,	0x0,	0x0,
	0x3,	0x0,	0xf,	0xb0,
	0x0,	0x0,	0xe4,	0xb4,
	0x4,	0x0,	0x0,	0x0,
	0x0,	0x0,	0x7,	0x80,
	0x3,	0x0,	0xe4,	0xb0,
	0x0,	0x0,	0xe4,	0x90,
	0x1,	0x0,	0xe4,	0x90,
	0x1,	0x0,	0x0,	0x0,
	0x0,	0x0,	0x8,	0x80,
	0x0,	0x0,	0xe4,	0x90,
	0xff,	0xff,	0x0,	0x0
	};

static const plShaderDecl ps_WaveFixedDecl("sha/ps_WaveFixed.inl", ps_WaveFixed, ps_WaveFixedbyteLen, ps_WaveFixedCodes);

static const plShaderRegister ps_WaveFixedRegister(&ps_WaveFixedDecl);

