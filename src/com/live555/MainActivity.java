package com.live555;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import com.live555.rtsp.RTSPClient;
import com.live555.rtsp.RTSPInfoListener;
import com.live555.rtsp.RTSPVideoListener;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {

	private VideoThread videoThread;
	private File file;
	private FileOutputStream fos;
	private boolean isrunning = false;

	static {
		try {
			System.loadLibrary("rtsplive555");
		} catch (Exception e) {
			e.printStackTrace();
		}

	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		Button btnStart = (Button) findViewById(R.id.btn_start);
		Button btnStop = (Button) findViewById(R.id.btn_stop);
		final Button btnShow = (Button) findViewById(R.id.btn_show);
		final TextView tvResult = (TextView) findViewById(R.id.tv_result);
		final ScrollView slv = (ScrollView) findViewById(R.id.slv);
		final EditText etAddress = (EditText) findViewById(R.id.et_address);
		etAddress.setSelection(etAddress.getText().toString().trim().length());

		// file = new File("/storage/emulated/0/rtsp.h264");
		//
		// Toast.makeText(getApplicationContext(), file.getAbsolutePath(),
		// Toast.LENGTH_LONG).show();
		//
		// try {
		// fos = new FileOutputStream(file);
		// } catch (FileNotFoundException e) {
		// e.printStackTrace();
		// }

		RTSPVideoListener videoListener = new RTSPVideoListener() {
			@Override
			public void videoCallBack(byte[] data, int len) {
//				Log.d("111111111111", data.length + " : " + len);
				if (file != null && fos != null && isrunning) {
					try {
						fos.write(data, 0, len);
						fos.flush();
					} catch (IOException e) {
						e.printStackTrace();
					}
				}
			}
		};
		RTSPClient.setRTSPVideoListener(videoListener);

		RTSPInfoListener infoListener = new RTSPInfoListener() {

			@Override
			public void infoCallBack(final String msg) {
				runOnUiThread(new Runnable() {
					public void run() {
						tvResult.append(msg + "\n");
						slv.fullScroll(ScrollView.FOCUS_DOWN);
					}
				});
			}
		};
		RTSPClient.setRTSPInfoListener(infoListener);

		btnStart.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				tvResult.setText("");

				videoThread = new VideoThread(etAddress.getText().toString().trim());
				videoThread.start();

				isrunning = true;

				btnShow.setEnabled(false);

				Toast.makeText(getApplicationContext(), "Start", Toast.LENGTH_SHORT).show();
			}
		});
		btnStop.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				RTSPClient.stop();

				isrunning = false;

				if (fos != null) {
					try {
						fos.close();
						fos = null;
					} catch (IOException e) {
						e.printStackTrace();
					}
				}

				btnShow.setEnabled(true);

				Toast.makeText(getApplicationContext(), "Stop", Toast.LENGTH_SHORT).show();
			}
		});
		btnShow.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				Intent intent = new Intent(MainActivity.this, MainActivity0.class);
				intent.putExtra("rtsp", etAddress.getText().toString().trim());
				startActivity(intent);
				finish();
			}
		});
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

		if (isrunning) {
			RTSPClient.stop();
		}
	}
}
