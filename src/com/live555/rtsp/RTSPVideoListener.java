package com.live555.rtsp;

public interface RTSPVideoListener {
	void videoCallBack(byte[] data, int len);
}
