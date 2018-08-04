#include <iostream>
#include <iostream>
#include "FrameStream.h"

#define FPS_BUFFER_NUM 1024
#define LOG_TAG "FrameAnimation"

using namespace std;

namespace frame_animation {
fpstream FPLog;

class fpsbuffer : public streambuf {
	android_LogPriority priority;
	string tag;
	char buffer[FPS_BUFFER_NUM];

	void set_priority (android_LogPriority pri) {
		if (priority != pri)
			flush_buffer();
		priority = pri;
	}

	int flush_buffer () {
		int num = pptr() - pbase();
		if (pptr() == nullptr || num <= 0)
			return 0;

		/* c style string */
		*pptr() = '\0';
		pbump(1);

		if (__android_log_print(priority, tag.c_str(), "%s", pbase()) < 0) {
			cerr<<"flush_buffer @__android_log_print"<<endl;
			pbump(-1);
			return EOF;
		}

		setp(buffer, buffer + FPS_BUFFER_NUM - 1);
		return num;
	}

	friend class fpstream;
public:
	fpsbuffer (string tag) {
		this->tag = tag;
		setp(buffer, buffer + FPS_BUFFER_NUM - 1);
	}

	virtual int overflow (int c = EOF) {
		if (c != EOF) {
			*pptr() = c;
			pbump(1);
		}

		if (flush_buffer() == EOF) {
			cerr<<"overflow @flush_buffer == EOF"<<endl;
			return EOF;
		}

		return c;
	}

	virtual int sync () {
		if (flush_buffer() == EOF) {
			cerr<<"sync @flush_buffer == EOF"<<endl;
			return -1;
		}

		return 0;
	}
};

// --------------------------------------------------------
fpstream::fpstream ():ostream(new fpsbuffer(LOG_TAG)) {}

fpstream& fpstream::log_priority (android_LogPriority pri) {
	dynamic_cast<fpsbuffer*>(rdbuf())->set_priority(pri);
	return *this;
}

// --------------------------------------------------------
ResStreamBuf::ResStreamBuf (shared_ptr<Asset> asset) {
	this->asset = asset;
	buf = const_cast<char*>(static_cast<const char*>(asset->getBuffer(true)));
	setg(buf, buf, buf + asset->getLength());
}

int ResStreamBuf::underflow () {
	return EOF;
}

int ResStreamBuf::uflow () {
	return EOF;
}

// --------------------------------------------------------
ZipStreamBuf::ZipStreamBuf (shared_ptr<ZipFileRO> zip, string name) {
	zip_file  = zip;
	file_name = name;

	ZipEntryRO entry = zip_file->findEntryByName(name.c_str());
	if (entry == nullptr) {
		FPLog.E()<<"open zipEntry "<<name<<" fail";
		return;
	}

	file_map = unique_ptr<FileMap>(zip_file->createEntryFileMap(entry));
	char* ptr = static_cast<char*>(file_map->getDataPtr());

	setg(ptr, ptr, ptr + file_map->getDataLength());
}

int ZipStreamBuf::underflow () {
	return EOF;
}

int ZipStreamBuf::uflow () {
	return EOF;
}

}; //namespace frame_animation
