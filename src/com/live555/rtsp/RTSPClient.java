package com.live555.rtsp;

import android.text.TextUtils;
import android.util.Log;

public class RTSPClient {
	private static RTSPVideoListener mVideoListener;
	private static RTSPInfoListener mInfoListener;

	public static native int start(String path);

	public void onNativeCallBack(byte[] data, int len) {
		if (mVideoListener != null) {
			mVideoListener.videoCallBack(data, len);
		}
	}

	public void onNativeInfo(String errorMsg) {
		if (TextUtils.isEmpty(errorMsg)) {
			return;
		}
		
		if(mInfoListener != null) {
			mInfoListener.infoCallBack(errorMsg);
		}
		
		Log.d("RTSPClient", errorMsg);
	}

	public static native void stop();

	public static void setRTSPVideoListener(RTSPVideoListener listener) {
		mVideoListener = listener;
	}
	
	public static void setRTSPInfoListener(RTSPInfoListener listener) {
		mInfoListener = listener;
	}
}
