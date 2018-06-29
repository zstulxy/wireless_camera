/**********
 This library is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the
 Free Software Foundation; either version 3 of the License, or (at your
 option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

 This library is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this library; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 **********/
// Copyright (c) 1996-2017, Live Networks, Inc.  All rights reserved
// A demo application, showing how to create and run a RTSP client (that can potentially receive multiple streams concurrently).
//
// NOTE: This code - although it builds a running application - is intended only to illustrate how to develop your own RTSP
// client application.  For a full-featured RTSP client application - with much more functionality, and many options - see
// "openRTSP": http://www.live555.com/openRTSP/
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include <string>
#include <sstream>

#include <jni.h>
#include <android/log.h>

#include <rtsp/com_live555_rtsp_RTSPClient.h>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_TAG "rtsplive555"
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Forward function definitions:

// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode,
		char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode,
		char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode,
		char* resultString);

// Other event handler functions:
void subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void subsessionByeHandler(void* clientData); // called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void* clientData);
// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

// The main streaming routine (for each "rtsp://" URL):
void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL);

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient* rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
//UsageEnvironment& operator<<(UsageEnvironment& env,
//		const RTSPClient& rtspClient) {
//	return env << "[URL:\"" << rtspClient.url() << "\"]: ";
//}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
//UsageEnvironment& operator<<(UsageEnvironment& env,
//		const MediaSubsession& subsession) {
//	return env << subsession.mediumName() << "/" << subsession.codecName();
//}

//void usage(UsageEnvironment& env, char const* progName) {
//	env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
//	env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
//}

char eventLoopWatchVariable = 0;
RTSPClient* _rtspClient;

JNIEnv *_env;

unsigned char runningFlag = 1;
unsigned char sps_pps[21] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x4D, 0x00, 0x1E,
		0x95, 0xA8, 0x28, 0x0F, 0x64, 0x00, 0x00, 0x00, 0x01, 0x68, 0xEE, 0x3C,
		0x80 };

jclass _clazz;
jobject _obj;
jmethodID _methodid1;
jmethodID _methodid2;

Boolean isrunning = false;
string tmp;

/**
 * c/c++ string turn to java jstring
 */
jstring charToJstring(JNIEnv* env, const char* pat) {
	if (env == NULL) {
		return NULL;
	}

	jclass _strclass = (env)->FindClass("java/lang/String");
	jstring _encode = (env)->NewStringUTF("GB2312");
	jmethodID ctorID = env->GetMethodID(_strclass, "<init>",
			"([BLjava/lang/String;)V");
	jbyteArray bytes = env->NewByteArray(strlen(pat));
	env->SetByteArrayRegion(bytes, 0, strlen(pat), (jbyte*) pat);
	jstring jstr = (jstring) env->NewObject(_strclass, ctorID, bytes, _encode);
//	env->ReleaseByteArrayElements(bytes, NULL, 0);
	env->DeleteLocalRef(bytes);
	env->DeleteLocalRef(_strclass);
	env->DeleteLocalRef(_encode);
	return jstr;
}

/**
 * info call back
 */
void infoCallBack(jstring jstr) {
	if (!isrunning) {
		return;
	}

	if (_obj == NULL && _clazz != NULL) {
		_obj = _env->AllocObject(_clazz);
	}

	if (_obj != NULL && _methodid1 != NULL) {
		_env->CallVoidMethod(_obj, _methodid1, jstr);
		_env->DeleteLocalRef(jstr);
	}

	tmp = "";
}

int start(const char* path) {
	// Begin by setting up our usage environment:
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

	// We need at least one "rtsp://" URL argument:
	if (path == NULL) {
//		LOGE("URL is not allow NULL!");
		tmp = "URL is not allow NULL!";
		infoCallBack(charToJstring(_env, tmp.c_str()));
		return 1;
	}

	eventLoopWatchVariable = 0;

	// There are argc-1 URLs: argv[1] through argv[argc-1].  Open and start streaming each one:
	openURL(*env, NULL, path);

	// All subsequent activity takes place within the event loop:
	env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
	// This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.

//  return 0;

	// If you choose to continue the application past this point (i.e., if you comment out the "return 0;" statement above),
	// and if you don't intend to do anything more with the "TaskScheduler" and "UsageEnvironment" objects,
	// then you can also reclaim the (small) memory used by these objects by uncommenting the following code:

	env->reclaim();
	env = NULL;
	delete scheduler;
	scheduler = NULL;

	return 0;
}

/*
 * Class:     com_live555_rtsp_RTSPClient
 * Method:    start
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_live555_rtsp_RTSPClient_start(JNIEnv *env,
		jclass jclz, jstring path) {
	if (!isrunning) {
		const char* sdp_title = env->GetStringUTFChars(path, NULL);

		_env = env;
		_clazz = jclz;
		runningFlag = 1;
		isrunning = true;

		_methodid1 = env->GetMethodID(jclz, "onNativeInfo",
				"(Ljava/lang/String;)V");
		if (_methodid1 == NULL) {
			LOGE(
					"can't find method: public void onNativeInfo(java.lang.String)");
		}

		start(sdp_title);

		env->ReleaseStringUTFChars(path, sdp_title);
		env->DeleteLocalRef(path);

		if (_clazz != NULL && _obj != NULL) {
			_env->DeleteLocalRef(_clazz);
			_env->DeleteLocalRef(_obj);
		}

		_methodid1 = NULL;
		_methodid2 = NULL;

		_clazz = NULL;
		_obj = NULL;

		_env = NULL;
		_rtspClient = NULL;

		return 0;
	} else {
		return -1;
	}
}

/*
 * Class:     com_live555_rtsp_RTSPClient
 * Method:    stop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_live555_rtsp_RTSPClient_stop(JNIEnv *env, jclass jclz)
{
	if(isrunning) {
		isrunning = false;

		if(_clazz != NULL && _obj != NULL) {
			_env->DeleteLocalRef(_clazz);
			_env->DeleteLocalRef(_obj);
		}

		_methodid1 = NULL;
		_methodid2 = NULL;

		_clazz = NULL;
		_obj = NULL;
	}
}

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:

class StreamClientState {
public:
	StreamClientState();
	virtual ~StreamClientState();

public:
	MediaSubsessionIterator* iter;
	MediaSession* session;
	MediaSubsession* subsession;
	TaskToken streamTimerTask;
	double duration;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:

class ourRTSPClient: public RTSPClient {
public:
	static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
			int verbosityLevel = 0, char const* applicationName = NULL,
			portNumBits tunnelOverHTTPPortNum = 0);

protected:
	ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
			int verbosityLevel, char const* applicationName,
			portNumBits tunnelOverHTTPPortNum);
	// called only by createNew();
	virtual ~ourRTSPClient();

public:
	StreamClientState scs;
};

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.

class DummySink: public MediaSink {
public:
	static DummySink* createNew(UsageEnvironment& env,
			MediaSubsession& subsession, // identifies the kind of data that's being received
			char const* streamId = NULL); // identifies the stream itself (optional)

private:
	DummySink(UsageEnvironment& env, MediaSubsession& subsession,
			char const* streamId);
	// called only by "createNew()"
	virtual ~DummySink();

	static void afterGettingFrame(void* clientData, unsigned frameSize,
			unsigned numTruncatedBytes, struct timeval presentationTime,
			unsigned durationInMicroseconds);
	void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
			struct timeval presentationTime, unsigned durationInMicroseconds);

private:
	// redefined virtual functions:
	virtual Boolean continuePlaying();

private:
	u_int8_t* fReceiveBuffer;
	MediaSubsession& fSubsession;
	char* fStreamId;
};

#define RTSP_CLIENT_VERBOSITY_LEVEL 1 // by default, print verbose output from each "RTSPClient"

//static unsigned rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.

void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL) {
	// Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
	// to receive (even if more than stream uses the same "rtsp://" URL).
	RTSPClient* rtspClient = ourRTSPClient::createNew(env, rtspURL,
	RTSP_CLIENT_VERBOSITY_LEVEL, progName);
	if (rtspClient == NULL) {
//		env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": "
//				<< env.getResultMsg() << "\n";
//		LOGE("Failed to create a RTSP client for URL \"%s\": %s", rtspURL,
//				env.getResultMsg());
		tmp = "Failed to create a RTSP client for URL \"" + string(rtspURL)
				+ "\":" + string(env.getResultMsg());
		infoCallBack(charToJstring(_env, tmp.c_str()));
		return;
	}

	_rtspClient = rtspClient;

//  ++rtspClientCount;

	// Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
	// Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
	// Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
	rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
}

// Implementation of the RTSP 'response handlers':

void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode,
		char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((ourRTSPClient*) rtspClient)->scs; // alias

		if (resultCode != 0) {
//			env << *rtspClient << "Failed to get a SDP description: "
//					<< resultString << "\n";
//			LOGE("Failed to get a SDP description: %s", resultString);
			tmp = "Failed to get a SDP description: " + string(resultString);
			infoCallBack(charToJstring(_env, tmp.c_str()));
			delete[] resultString;
			break;
		}

		char* const sdpDescription = resultString;

//		env << *rtspClient << "Got a SDP description:\n" << sdpDescription
//				<< "\n";
//		LOGD("Got a SDP description:\n%s", sdpDescription);
		tmp = "Got a SDP description:\n" + string(resultString);
		infoCallBack(charToJstring(_env, tmp.c_str()));

		// Create a media session object from this SDP description:
		scs.session = MediaSession::createNew(env, sdpDescription);
		delete[] sdpDescription; // because we don't need it anymore
		if (scs.session == NULL) {
//			env << *rtspClient
//					<< "Failed to create a MediaSession object from the SDP description: "
//					<< env.getResultMsg() << "\n";
//			LOGE(
//					"Failed to create a MediaSession object from the SDP description: %s",
//					env.getResultMsg());
			tmp =
					"Failed to create a MediaSession object from the SDP description: "
							+ string(env.getResultMsg());
			infoCallBack(charToJstring(_env, tmp.c_str()));
			break;
		} else if (!scs.session->hasSubsessions()) {
//			env << *rtspClient
//					<< "This session has no media subsessions (i.e., no \"m=\" lines)\n";
//			LOGE(
//					"This session has no media subsessions (i.e., no \"m=\" lines)");
			tmp =
					"This session has no media subsessions (i.e., no \"m=\" lines)";
			infoCallBack(charToJstring(_env, tmp.c_str()));
			break;
		}

		// Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
		// calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
		// (Each 'subsession' will have its own data source.)
		scs.iter = new MediaSubsessionIterator(*scs.session);
		setupNextSubsession(rtspClient);
		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	shutdownStream(rtspClient);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP False

void setupNextSubsession(RTSPClient* rtspClient) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((ourRTSPClient*) rtspClient)->scs; // alias

	scs.subsession = scs.iter->next();
	if (scs.subsession != NULL) {
		if (!scs.subsession->initiate()) {
//			env << *rtspClient << "Failed to initiate the \"" << *scs.subsession
//					<< "\" subsession: " << env.getResultMsg() << "\n";
//			LOGE("Failed to initiate the subsession: %s", env.getResultMsg());
			tmp = "Failed to initiate the subsession: "
					+ string(env.getResultMsg());
			infoCallBack(charToJstring(_env, tmp.c_str()));
			setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
		} else {
//			env << *rtspClient << "Initiated the \"" << *scs.subsession
//					<< "\" subsession (";
//			LOGD("Initiated the subsession (");
			tmp = "Initiated the subsession (";
			ostringstream oss;
			if (scs.subsession->rtcpIsMuxed()) {
//				env << "client port " << scs.subsession->clientPortNum();
//				LOGD("client port %d", scs.subsession->clientPortNum());
				oss << tmp << "client port " << scs.subsession->clientPortNum()
						<< ")";
				tmp = oss.str();
			} else {
//				env << "client ports " << scs.subsession->clientPortNum() << "-"
//						<< scs.subsession->clientPortNum() + 1;
//				LOGD("client port %d-%d", scs.subsession->clientPortNum(),
//						scs.subsession->clientPortNum() + 1);
				oss << tmp << "client port " << scs.subsession->clientPortNum()
						<< "-" << scs.subsession->clientPortNum() + 1 << ")";
				tmp = oss.str();
			}
//			env << ")\n";
			infoCallBack(charToJstring(_env, tmp.c_str()));

			// Continue setting up this subsession, by sending a RTSP "SETUP" command:
			rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP,
					False, REQUEST_STREAMING_OVER_TCP);
		    tmp = "sendSetupCommand continueAfterSETUP ------------------";
		    infoCallBack(charToJstring(_env, tmp.c_str()));
		}
		return;
	}

	// We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
	if (scs.session->absStartTime() != NULL) {
		// Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
		rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY,
				scs.session->absStartTime(), scs.session->absEndTime());
	} else {
		scs.duration = scs.session->playEndTime()
				- scs.session->playStartTime();
		rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
	}
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode,
		char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((ourRTSPClient*) rtspClient)->scs; // alias

		if (resultCode != 0) {
//			env << *rtspClient << "Failed to set up the \"" << *scs.subsession
//					<< "\" subsession: " << resultString << "\n";
//			LOGE("Failed to set up the subsession: %s", resultString);
			tmp = "Failed to set up the subsession: " + string(resultString);
			infoCallBack(charToJstring(_env, tmp.c_str()));
			break;
		}

//		env << *rtspClient << "Set up the \"" << *scs.subsession
//				<< "\" subsession (";
//		LOGD("Set up the subsession (");
		tmp = "Set up the subsession (";
		ostringstream oss;
		if (scs.subsession->rtcpIsMuxed()) {
//			env << "client port " << scs.subsession->clientPortNum();
//			LOGD("client port %d", scs.subsession->clientPortNum());
			oss << tmp << "client port " << scs.subsession->clientPortNum()
					<< ")";
			tmp = oss.str();
		} else {
//			env << "client ports " << scs.subsession->clientPortNum() << "-"
//					<< scs.subsession->clientPortNum() + 1;
//			LOGD("client port %d-%d", scs.subsession->clientPortNum(),
//					scs.subsession->clientPortNum() + 1);
			oss << tmp << "client port " << scs.subsession->clientPortNum()
					<< "-" << scs.subsession->clientPortNum() + 1 << ")";
			tmp = oss.str();
		}
//		env << ")\n";
		infoCallBack(charToJstring(_env, tmp.c_str()));

		// Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
		// (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
		// after we've sent a RTSP "PLAY" command.)

		scs.subsession->sink = DummySink::createNew(env, *scs.subsession,
				rtspClient->url());
		tmp = "url: " + std::string(rtspClient->url());
	    infoCallBack(charToJstring(_env, tmp.c_str()));
		// perhaps use your own custom "MediaSink" subclass instead
		if (scs.subsession->sink == NULL) {
//			env << *rtspClient << "Failed to create a data sink for the \""
//					<< *scs.subsession << "\" subsession: "
//					<< env.getResultMsg() << "\n";
//			LOGE("Failed to create a data sink for the subsession: %s",
//					env.getResultMsg());
			tmp = "Failed to create a data sink for the subsession: "
					+ string(env.getResultMsg());
			infoCallBack(charToJstring(_env, tmp.c_str()));
			break;
		}

//		env << *rtspClient << "Created a data sink for the \""
//				<< *scs.subsession << "\" subsession\n";
//		LOGD("Created a data sink for the subsession");
		tmp = "Created a data sink for the subsession";
		infoCallBack(charToJstring(_env, tmp.c_str()));
		scs.subsession->miscPtr = rtspClient;// a hack to let subsession handler functions get the "RTSPClient" from the subsession
		scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
				subsessionAfterPlaying, scs.subsession);
		tmp = "startPlaying--------------";
		infoCallBack(charToJstring(_env, tmp.c_str()));
		// Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
		if (scs.subsession->rtcpInstance() != NULL) {
			scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler,
					scs.subsession);
		}
	} while (0);
	delete[] resultString;

// Set up the next subsession, if any:
	setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode,
		char* resultString) {
	Boolean success = False;
	ostringstream oss;

	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((ourRTSPClient*) rtspClient)->scs; // alias

		if (resultCode != 0) {
//			env << *rtspClient << "Failed to start playing session: "
//					<< resultString << "\n";
//			LOGE("Failed to start playing session: %s", resultString);
			tmp = "Failed to start playing session: " + string(resultString);
			infoCallBack(charToJstring(_env, tmp.c_str()));
			break;
		}

		// Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
		// using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
		// 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
		// (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
		if (scs.duration > 0) {
			unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
			scs.duration += delaySlop;
			unsigned uSecsToDelay = (unsigned) (scs.duration * 1000000);
			scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(
					uSecsToDelay, (TaskFunc*) streamTimerHandler, rtspClient);
		}

//		env << *rtspClient << "Started playing session";
//		LOGD("Started playing session");
		tmp = "Started playing session";
		infoCallBack(charToJstring(_env, tmp.c_str()));
		if (scs.duration > 0) {
//			env << " (for up to " << scs.duration << " seconds)";
//			LOGD("(for up to %f seconds)...", scs.duration);
			oss << "(for up to " << scs.duration << " seconds)...";
			tmp = oss.str();
			infoCallBack(charToJstring(_env, tmp.c_str()));
		}
//		env << "...\n";

		success = True;
	} while (0);
	delete[] resultString;

	if (!success) {
		// An unrecoverable error occurred with this stream.
		shutdownStream(rtspClient);
	}
}

// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) {
	MediaSubsession* subsession = (MediaSubsession*) clientData;
	RTSPClient* rtspClient = (RTSPClient*) (subsession->miscPtr);

// Begin by closing this subsession's stream:
	Medium::close(subsession->sink);
	subsession->sink = NULL;

// Next, check whether *all* subsessions' streams have now been closed:
	MediaSession& session = subsession->parentSession();
	MediaSubsessionIterator iter(session);
	while ((subsession = iter.next()) != NULL) {
		if (subsession->sink != NULL)
			return; // this subsession is still active
	}

// All subsessions' streams have now been closed, so shutdown the client:
	shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData) {
	MediaSubsession* subsession = (MediaSubsession*) clientData;
	RTSPClient* rtspClient = (RTSPClient*) subsession->miscPtr;
	UsageEnvironment& env = rtspClient->envir(); // alias

//	env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession
//			<< "\" subsession\n";
//	LOGD("Received RTCP \"BYE\" on subsession");
	tmp = "Received RTCP \"BYE\" on subsession";
	infoCallBack(charToJstring(_env, tmp.c_str()));

// Now act as if the subsession had closed:
	subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
	ourRTSPClient* rtspClient = (ourRTSPClient*) clientData;
	StreamClientState& scs = rtspClient->scs; // alias

	scs.streamTimerTask = NULL;

// Shut down the stream:
	shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((ourRTSPClient*) rtspClient)->scs; // alias

// First, check whether any subsessions have still to be closed:
	if (scs.session != NULL) {
		Boolean someSubsessionsWereActive = False;
		MediaSubsessionIterator iter(*scs.session);
		MediaSubsession* subsession;

		while ((subsession = iter.next()) != NULL) {
			if (subsession->sink != NULL) {
				Medium::close(subsession->sink);
				subsession->sink = NULL;

				if (subsession->rtcpInstance() != NULL) {
					subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
				}

				someSubsessionsWereActive = True;
			}
		}

		if (someSubsessionsWereActive) {
			// Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
			// Don't bother handling the response to the "TEARDOWN".
			rtspClient->sendTeardownCommand(*scs.session, NULL);
		}
	}

//	env << *rtspClient << "Closing the stream.\n";
//	LOGD("Closing the stream.");
	tmp = "Closing the stream.";
	infoCallBack(charToJstring(_env, tmp.c_str()));
	Medium::close(rtspClient);
// Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

	eventLoopWatchVariable = 1;

	isrunning = false;

	if (_clazz != NULL && _obj != NULL) {
		_env->DeleteLocalRef(_clazz);
		_env->DeleteLocalRef(_obj);
	}

	_methodid1 = NULL;
	_methodid2 = NULL;

	_clazz = NULL;
	_obj = NULL;

//  if (--rtspClientCount == 0) {
// The final stream has ended, so exit the application now.
// (Of course, if you're embedding this code into your own application, you might want to comment this out,
// and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
//	exit(exitCode);
//  }
}

// Implementation of "ourRTSPClient":

ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env,
		char const* rtspURL, int verbosityLevel, char const* applicationName,
		portNumBits tunnelOverHTTPPortNum) {
	return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName,
			tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName,
		portNumBits tunnelOverHTTPPortNum) :
		RTSPClient(env, rtspURL, verbosityLevel, applicationName,
				tunnelOverHTTPPortNum, -1) {
}

ourRTSPClient::~ourRTSPClient() {
}

// Implementation of "StreamClientState":

StreamClientState::StreamClientState() :
		iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(
				0.0) {
}

StreamClientState::~StreamClientState() {
	delete iter;
	if (session != NULL) {
		// We also need to delete "session", and unschedule "streamTimerTask" (if set)
		UsageEnvironment& env = session->envir(); // alias

		env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
		Medium::close(session);
	}
}

// Implementation of "DummySink":

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 100000

DummySink* DummySink::createNew(UsageEnvironment& env,
		MediaSubsession& subsession, char const* streamId) {
	return new DummySink(env, subsession, streamId);
}

DummySink::DummySink(UsageEnvironment& env, MediaSubsession& subsession,
		char const* streamId) :
		MediaSink(env), fSubsession(subsession) {
	fStreamId = strDup(streamId);
	fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
}

DummySink::~DummySink() {
	delete[] fReceiveBuffer;
	delete[] fStreamId;
}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize,
		unsigned numTruncatedBytes, struct timeval presentationTime,
		unsigned durationInMicroseconds) {
	DummySink* sink = (DummySink*) clientData;
	sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime,
			durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1

int size = 0;
unsigned char *buffer = NULL;

int len = 0;
unsigned char *temp_sps = NULL;
unsigned char *temp_sps_pps = NULL;

void DummySink::afterGettingFrame(unsigned frameSize,
		unsigned numTruncatedBytes, struct timeval presentationTime,
		unsigned /*durationInMicroseconds*/) {
// We've just received a frame of data.  (Optionally) print out information about it:
//#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
//	if (fStreamId != NULL)
//		envir() << "Stream \"" << fStreamId << "\"; ";
//	envir() << fSubsession.mediumName() << "/" << fSubsession.codecName()
//			<< ":\tReceived " << frameSize << " bytes";
//	if (numTruncatedBytes > 0)
//		envir() << " (with " << numTruncatedBytes << " bytes truncated)";
//	char uSecsStr[6 + 1]; // used to output the 'microseconds' part of the presentation time
//	sprintf(uSecsStr, "%06u", (unsigned) presentationTime.tv_usec);
//	envir() << ".\tPresentation time: " << (int) presentationTime.tv_sec << "."
//			<< uSecsStr;
//	if (fSubsession.rtpSource() != NULL
//			&& !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
//		envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
//	}
//#ifdef DEBUG_PRINT_NPT
//	envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
//#endif
//	envir() << "\n";
//#endif

	// only get the video data, exclude audio data.
	if (strcmp(fSubsession.mediumName(), "video") == 0) {
		if (runningFlag == 1) {
			// sps
			if (fReceiveBuffer[0] == 0x67) {
				len = frameSize + 4;
				temp_sps = (unsigned char *) malloc(len);
				temp_sps[0] = 0x00;
				temp_sps[1] = 0x00;
				temp_sps[2] = 0x00;
				temp_sps[3] = 0x01;
				memcpy(temp_sps + 4, fReceiveBuffer, frameSize);
			}

			// sps + pps
			if (fReceiveBuffer[0] == 0x68) {
				int l = len;
				len = len + frameSize + 4;

				temp_sps_pps = (unsigned char *) malloc(len);
				memcpy(temp_sps_pps, temp_sps, l);

				temp_sps_pps[l + 0] = 0x00;
				temp_sps_pps[l + 1] = 0x00;
				temp_sps_pps[l + 2] = 0x00;
				temp_sps_pps[l + 3] = 0x01;
				memcpy(temp_sps_pps + l + 4, fReceiveBuffer, frameSize);

				l = 0;
				free(temp_sps);
				temp_sps = NULL;
			}

			if (fReceiveBuffer[0] == 0) {
				runningFlag = 0;
				if (_methodid2 == NULL) {
					if (_clazz == NULL) {
						_clazz = _env->FindClass("com/live555/rtsp/RTSPClient");
						if (_clazz == NULL) {
							//						LOGE(
							//								"can't find class: \"com/live555/rtsp/RTSPClient\"");
							tmp =
									"can't find class: \"com/live555/rtsp/RTSPClient\"";
							infoCallBack(charToJstring(_env, tmp.c_str()));
						}
					}
					if (_obj == NULL && _clazz != NULL) {
						_obj = _env->AllocObject(_clazz);
					}

					_methodid2 = _env->GetMethodID(_clazz, "onNativeCallBack",
							"([BI)V");
					if (_methodid2 == NULL) {
						//					LOGE(
						//							"can't find method: public void onNativeCallBack(byte[], int)");
						tmp =
								"can't find method: public void onNativeCallBack(byte[], int)";
						infoCallBack(charToJstring(_env, tmp.c_str()));
					} else {
						if (len == 0) {
							jbyteArray jbarray = _env->NewByteArray(21);
							_env->SetByteArrayRegion(jbarray, 0, 21,
									(jbyte*) sps_pps);
							_env->CallVoidMethod(_obj, _methodid2, jbarray, 21);
							_env->DeleteLocalRef(jbarray);
						} else {
							jbyteArray jbarray = _env->NewByteArray(len);
							_env->SetByteArrayRegion(jbarray, 0, len,
									(jbyte*) temp_sps_pps);
							_env->CallVoidMethod(_obj, _methodid2, jbarray,
									len);
							_env->DeleteLocalRef(jbarray);

							len = 0;
							free(temp_sps_pps);
							temp_sps_pps = NULL;
						}
					}
				}
				size = frameSize + 4;
				buffer = (unsigned char *) malloc(size);
				buffer[0] = 0x00;
				buffer[1] = 0x00;
				buffer[2] = 0x00;
				buffer[3] = 0x01;
				memcpy(buffer + 4, fReceiveBuffer, frameSize);

				if (runningFlag == 0 && _methodid2 != NULL) {
					jbyteArray jbarray = _env->NewByteArray(size);
					_env->SetByteArrayRegion(jbarray, 0, size, (jbyte*) buffer);

					_env->CallVoidMethod(_obj, _methodid2, jbarray, size);
					_env->DeleteLocalRef(jbarray);
				}

				/*
				 ����Nalu_type:
				 0x67 (0 11 00111) SPS          �ǳ���Ҫ     type = 7
				 0x68 (0 11 01000) PPS          �ǳ���Ҫ     type = 8
				 0x65 (0 11 00101) IDR  �ؼ�֡      �ǳ���Ҫ      type = 5
				 0x61 (0 10 00001) I֡      ��Ҫ            type = 1
				 0x41 (0 10 00001) P֡      ��Ҫ            type = 1
				 0x01 (0 00 00001) B֡      ����Ҫ       type = 1
				 0x06 (0 00 00110) SEI  ����Ҫ       type = 6
				 */
			}
		} else {
			if (fReceiveBuffer[0] != 0x67 && fReceiveBuffer[0] != 0x68) {
				size = frameSize + 4;
				buffer = (unsigned char *) malloc(size);
				buffer[0] = 0x00;
				buffer[1] = 0x00;
				buffer[2] = 0x00;
				buffer[3] = 0x01;
				memcpy(buffer + 4, fReceiveBuffer, frameSize);

				if (runningFlag == 0 && _methodid2 != NULL) {
					jbyteArray jbarray = _env->NewByteArray(size);
					_env->SetByteArrayRegion(jbarray, 0, size, (jbyte*) buffer);

					_env->CallVoidMethod(_obj, _methodid2, jbarray, size);
					_env->DeleteLocalRef(jbarray);
				}
			}
		}

		free(buffer);
		size = 0;
		buffer = NULL;
	}

// Then continue, to request the next frame of data:
	if (!isrunning) {
		shutdownStream(_rtspClient);
	} else {
		continuePlaying();
	}
}

Boolean DummySink::continuePlaying() {
	if (fSource == NULL)
		return False; // sanity check (should not happen)

// Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
	fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
			afterGettingFrame, this, onSourceClosure, this);
	return True;
}

#ifdef __cplusplus
}
#endif
