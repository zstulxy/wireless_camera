package com.live555;

import com.live555.rtsp.RTSPClient;
import com.live555.rtsp.RTSPVideoListener;

import android.app.Activity;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.SurfaceView;
import android.view.Window;
import android.view.WindowManager;

public class MainActivity0 extends Activity {

	private VideoThread videoThread;
	private SurfaceView sfv;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
		setContentView(R.layout.activity_main0);

		sfv = (SurfaceView) findViewById(R.id.sfv);
		ViedioUtils.LocalShow.addView(sfv);

		RTSPVideoListener videoListener = new RTSPVideoListener() {
			@Override
			public void videoCallBack(byte[] data, int len) {
				ViedioUtils.makeSpsPps(data);
			}
		};
		RTSPClient.setRTSPVideoListener(videoListener);

		String rtsp = getIntent().getStringExtra("rtsp");

		if (!TextUtils.isEmpty(rtsp)) {
			videoThread = new VideoThread(rtsp);
			videoThread.start();
		}
	}

	private static class VideoThread extends Thread {

		private String url;

		public VideoThread(String path) {
			url = path;
		}

		@Override
		public void run() {
			super.run();
			RTSPClient.start(url);
		}
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();

		RTSPClient.stop();
		ViedioUtils.LocalShow.removeView();
	}
}
