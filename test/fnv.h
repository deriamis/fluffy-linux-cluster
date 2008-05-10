
// 32-bit FNV hash...
struct FnvHash {
	typedef unsigned long hashtype;
	hashtype hash;

	static const hashtype prime = 0x01000193;
	static const hashtype init = 0x811c9dc5;

	FnvHash() : hash(init) {
	}

	void addData(const unsigned char *data, int datalen) {
		for (int i=0; i<datalen; i++) {
			// fnv-1 has xor,multiply,
			// fnv has multiply,xor
			hash = hash * prime;
			hash = hash ^ ((hashtype) data[i]);
		}
	}
	void addData(const char *data, int datalen) {
		addData((const unsigned char *) data, datalen);
	};

	unsigned short get16() const {
		unsigned short s;
		// bottom 16 bits
		s = (unsigned short) (hash & 0xffff);
		// xor top 16 bits
		s ^= (unsigned short) (hash >> 16);
		return s;
	}
	hashtype get32() const {
		return hash;
	}

	hashtype get32rev() const {
		hashtype hin = hash, hout = 0 ;
		for (int i=0; i<32; i++) {
			hout = hout << 1;
			if (hin & 1) {
				hout |= 1;
			}
			hin = hin >> 1;
		}
		return hout;
	}
	unsigned short get16rev() const {
		hashtype rev = get32rev();
		unsigned short s;
		// bottom 16 bits
		s = (unsigned short) (rev & 0xffff);
		// xor top 16 bits
		s ^= (unsigned short) (rev >> 16);
		return s;
	}
};

