package com.jdodev.emilie;

import java.util.ArrayList;
import java.util.List;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.WifiLock;
import android.os.Build;
import android.os.PowerManager;
import android.util.Log;


@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
public class APSynch
{

	enum STATE {
		UNUSABLE,
		IDLE,
		CAPTURING,
		SYNC,
	}
	
	/** Get APSynch Status
	 * @return
	 * UNSUABLE : Android device is incompatible, or contructor error
	 * IDLE : use start(BSSID) method to start it
	 * SYNC : APSynch is started and synchronized 
	 */
	public STATE  getStatus()            { return mSTATE; }

	/** Get AP BSSID
	 * @return AP MAC address or null if not sync
	 */
	public String getBSSID()             { return (null==mTargetInfo)?null:mTargetInfo.BSSID; }
	
	/** Get AP CHANNEL
	 * @return AP Frequency in MHz or 0 if not sync
	 */
	public int    getFreq()              { return (null==mTargetInfo)?0:mTargetInfo.Channel; }
	
	/** Get Synchronized date (in ms)
	 * @return current date or 0 if not sync
	 */
	public long   getTimeInMiliseconds() { return (null==mTargetInfo || null==mTargetInfo.TimeConvert)?0:mTargetInfo.TimeConvert.val(System.currentTimeMillis()); } 
	
	
	/** Get last known data
	 * @return last data received
	 */
	public String   getData() { return (null==mTargetInfo)?null:mTargetInfo.LastData; } 

	/** Get Android Device Scan duration
	 * @return Number of miliseconds between each scan
	 */
	public long    getScanDuration()         { return (null==mDeviceInfo)?0:mDeviceInfo.ScanDuration; }
	
	/** Get Android Device Scan duration error
	 * @return Number of miliseconds between getScanDuration() value and farest value 
	 */
	public long    getScanDurationError()         { return (null==mDeviceInfo)?0:mDeviceInfo.ScanDurationError; }
	
	// Listeners  
	public static final int CAPTURE_AP_MISSING     = 10;
	public static final int CAPTURE_SCANERROR      = 11;
	public static final int CAPTURE_SYNCERROR      = 12;
	public static final int CAPTURE_AP_UNTRUSTABLE = 13;
	
	public interface EventsListener
	{
		/** AP is synchronized event! */
		public void onAPSync();
		
		/** AP is no more synchronized event! */
		public void onAPOutOfSync();
		
		/** AP have send data event! */
		public void onAPEvent(String data);
		
		/** An error appear see error code above */
		public void onAPError(int errorCode);
	}

	
	
	
	
	
	/* 802.11 Beacon Frame:
	 *		 BSSID     : 24bits, AP MAC identifier (used as reference)
	 *       TimeStamp : 64bits, Number of microseconds timekeeper has been active ( used for time synchronized )
	 *		 SSID      : 1-32 Bytes (used for data)       
	 */
	
	
	// ----------------------------------------------------------------------
	// TODO find way to perform fast wifiscan ???
	// Based on the fact that
	//       1- We known the AP channel
	//       2- We don't need to wait so long neither (max scan time = 250ms ?)
	// ----------------------------------------------------------------------
	protected void fastscan() {
		if (mMainWifiObj!=null) mMainWifiObj.startScan();
	}
	// ----------------------------------------------------------------------	

	
	/* Config */
	protected static final String LOG_TAG = "APSYNC";	
	protected static final int CAPTURE_BEACON_CNT = 4; // nb of beacons used for really start sync
	protected static final int SYNC_BEACON_CNT = 10;   // nb max of beacons to compute sync
	protected static final int SYNC_OUTBEACON_CNT = 4; // nb max of beacons missing before consider the AP offline

	
	/* ctx, handler, listerners */
	protected Context mCtx = null;
	protected EventsListener mListener = null;
	protected STATE mSTATE = STATE.UNUSABLE;
	protected PowerManager.WakeLock mWakeLock = null;
	protected WifiManager mMainWifiObj = null;
	protected WifiLock mWifiLock = null;
	protected WifiScanReceiver mWifiReceiver = null;
	
	/* Targeted AP info */
	protected TargetInfo mTargetInfo = null;
	protected class TargetInfo {
		public String         BSSID;
		public int            Channel;
		public String         LastData;
		public FLine          TimeConvert;
		public List<SyncInfo> SyncInfo;		
	}
	
	/* Device Info  */
	protected DeviceInfo mDeviceInfo = null;
	protected class DeviceInfo {
		public CaptureInfo[] CaptureInfo = null;
		public long ScanDuration = 0;
		public long ScanDurationError = 0;
	}
	
	

	/* Unique constructor */	
	@SuppressLint("NewApi")
	public APSynch(Context ctx, EventsListener listener)
	{
		mCtx = ctx;
		mListener = listener;
		if ((null==mCtx) || (null==mListener)) {
			release();
			return;
		}
		
		// keep power ON
        final PowerManager pm = (PowerManager) mCtx.getSystemService(Context.POWER_SERVICE);
        this.mWakeLock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK, "EMILIE_TAG");
        this.mWakeLock.acquire();

		// WIFI !
        mMainWifiObj = (WifiManager) mCtx.getSystemService(Context.WIFI_SERVICE); 
        mMainWifiObj.disconnect();
        if (!mMainWifiObj.isScanAlwaysAvailable()) {
        	release();
        	return;
        }
        mMainWifiObj.setWifiEnabled(true);
        mWifiLock = mMainWifiObj.createWifiLock( WifiManager.WIFI_MODE_SCAN_ONLY, "EMILIE_TAG");
        mWifiLock.acquire();
        
        // Wifi Scan receiver
        mWifiReceiver = new WifiScanReceiver();
        mCtx.registerReceiver(mWifiReceiver, new IntentFilter(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION));
        
        mSTATE = STATE.IDLE;
        
        
        /* Some tests */
        /*JavaReflect.PrintClass(android.net.wifi.WifiManager.class);
        JavaReflect.GetField(mMainWifiObj, "mService");*/         
	}
	
	/* APSync release (can be restart after) */
	public void release()
	{
		 if (null!=mWifiLock)     mWifiLock.release();
		 if (null!=mWakeLock)     mWakeLock.release();
		 if ((null!=mWifiReceiver) && (null!=mCtx)) mCtx.unregisterReceiver(mWifiReceiver);
		 
		 mCtx = null;
		 mListener = null;
		 mSTATE = STATE.UNUSABLE;
		 mWakeLock = null;
		 mMainWifiObj = null;
		 mWifiLock = null;
		 mWifiReceiver = null;
		 mTargetInfo = null;
		 mDeviceInfo = null;
	}
	
	
	
	/* CAPTURING phase */
	
	protected static int mCurrCapIdx = 0; 
	
	protected class CaptureInfo {
		public long date; /* reception date in milliseconds */ 
		public List<ScanResult> wifiScanList; /* list of Wifi beacon */		
	}
	
	
	/** Start an synchronization with a an WiFi AccessPoint
	 * 
	 * @param BSSID : specific AP, or first one if null
	 * @return false if failed
	 */
	public Boolean start(String BSSID) {
		if (STATE.IDLE != mSTATE) return false;
		
		mDeviceInfo = new DeviceInfo();
		mDeviceInfo.CaptureInfo = new CaptureInfo[CAPTURE_BEACON_CNT];
		mDeviceInfo.ScanDuration = 0;
		mDeviceInfo.ScanDurationError = 0;
		
		mTargetInfo = new TargetInfo();
		mTargetInfo.BSSID = BSSID;
		mTargetInfo.Channel = 0;
		mTargetInfo.SyncInfo = null;
		mTargetInfo.LastData = "";
		mTargetInfo.TimeConvert = null;

		mCurrCapIdx = 0;
		  
		if (mMainWifiObj.startScan()) mSTATE = STATE.CAPTURING;
		return (STATE.CAPTURING==mSTATE);
	}
	
	private void onCapturingBeacons(List<ScanResult> beacons)
	{
		if (STATE.CAPTURING != mSTATE) return;
		
		// push element in the list
		CaptureInfo elt = new CaptureInfo();
		elt.date = System.currentTimeMillis();
		elt.wifiScanList = beacons;
		mDeviceInfo.CaptureInfo[mCurrCapIdx++] = elt;
		
		if (mCurrCapIdx >= CAPTURE_BEACON_CNT) {
			
			// Automatically take the first AP if not specified
			if (mTargetInfo.BSSID==null) {
				mTargetInfo.BSSID=beacons.get(0).BSSID;
				mTargetInfo.Channel=beacons.get(0).frequency;
			}

			// 1- figure out device scan duration
			// 1.1 delta list
			long delta[] = new long[CAPTURE_BEACON_CNT-1];
			for(int i=0; i<CAPTURE_BEACON_CNT; i++) {
				if (i>0) delta[i-1] = mDeviceInfo.CaptureInfo[i].date - mDeviceInfo.CaptureInfo[i-1].date; 
			}
			
			// 1.2 mean value
			long sum = 0;
			for(int i=0; i<CAPTURE_BEACON_CNT-1; i++) sum += delta[i];
			long mean  = sum/(CAPTURE_BEACON_CNT-1); // mean value
			
			// 1.3 max diff
			long maxdiff  = 0;
			for(int i=0; i<CAPTURE_BEACON_CNT-1; i++) {
				long diff = Math.abs(mean - delta[i]);
				if (diff>maxdiff) maxdiff = diff; 
			}
			
			// 1.4 check
			if (((float)maxdiff/(float)mean)>0.25f) {
				mSTATE = STATE.IDLE;
				mListener.onAPError(CAPTURE_AP_UNTRUSTABLE);
				return;
			}
			
			mDeviceInfo.ScanDuration = mean;
			mDeviceInfo.ScanDurationError = maxdiff;

			Log.i(LOG_TAG, "Device scan duration:" + mDeviceInfo.ScanDuration+" ms");
			Log.i(LOG_TAG, "Device scan duration error:" + mDeviceInfo.ScanDurationError+" ms");
			Log.i(LOG_TAG, "Target AP:" + beacons.get(0).SSID);
			Log.i(LOG_TAG, "Target AP:" + mTargetInfo.BSSID+" "+mTargetInfo.Channel + "MHz");
						
			// 2- Find expected BSSID
			for(int i=0; i<CAPTURE_BEACON_CNT; i++)
			{
				boolean found = false;
				for(ScanResult sr : mDeviceInfo.CaptureInfo[i].wifiScanList) {
					if (sr.BSSID.compareTo(mTargetInfo.BSSID)==0) {
						found = true;
						break;
					}
				}
				if (!found) {
					mSTATE = STATE.IDLE;
					mListener.onAPError(CAPTURE_AP_MISSING);
					return;
				}
			}
			
			// 3- Copying info on SyncInfo!
			mTargetInfo.SyncInfo = new ArrayList<SyncInfo>();
			for(int i=0; i<CAPTURE_BEACON_CNT; i++) {
				SyncInfo item = new SyncInfo();
				item.sysdate = mDeviceInfo.CaptureInfo[i].date;
				for(ScanResult sr : mDeviceInfo.CaptureInfo[i].wifiScanList) {
					if (sr.BSSID.compareTo(mTargetInfo.BSSID)==0) {
						item.beacon = sr;
						item.apdate = sr.timestamp/1000L; /* in Milliseconds */
						break;
					}
				}
				item.sysdiff = 0;
				item.apdiff = 0;
				mTargetInfo.SyncInfo.add(item);
			}
			
			// 4- Compute Regression line	
			mTargetInfo.TimeConvert = new FLine();
			if (ComputeLinearRegression(mTargetInfo.SyncInfo, mTargetInfo.TimeConvert)) {
				mSTATE = STATE.SYNC;
				mListener.onAPSync();
			} else {
				mSTATE = STATE.IDLE;
				mListener.onAPError(CAPTURE_SYNCERROR);
			}
			
		} else {
			// ask for a new beacon
			if (!mMainWifiObj.startScan()) {
				mSTATE = STATE.IDLE;
				mListener.onAPError(CAPTURE_SCANERROR);
				return;
			}
		}

		
		
	}
	
	
	/* SYNC phase */

	protected class SyncInfo {
		public String toString() {
			String m = "SyncInfo ";
			m+="sysdate:" + sysdate + "ms (" + sysdiff + ")";
			m+="apdate:" + sysdate + "ms (" + apdiff + ")";
			return m;
		}
		
		public ScanResult beacon; /* Wifi beacon */
		
		public long sysdate;      /* system reception date in milliseconds */
		public long apdate;       /* ap date in milliseconds */
		
		public long sysdiff;      /* sysdate diff with the previous beacon */
		public long apdiff;       /* apdiff diff with the previous beacon */
	}
	
	/** Stop the synchronization
	 */
	public void stop() {
		mSTATE = STATE.IDLE;
		mListener.onAPOutOfSync();
	}
	
	private void onSyncBeacons(ScanResult beacon) {
		
		SyncInfo item = new SyncInfo();
		item.beacon  = beacon;
		item.sysdate = System.currentTimeMillis();
		item.apdate  = beacon.timestamp/1000L; /* in Milliseconds */
		item.sysdiff = 0;
		item.apdiff = 0;
		mTargetInfo.SyncInfo.add(item);
		
		if(mTargetInfo.SyncInfo.size() > SYNC_BEACON_CNT) {
			mTargetInfo.SyncInfo.remove(0);
		}
		
		// Compute Regression line	
		if (!ComputeLinearRegression(mTargetInfo.SyncInfo, mTargetInfo.TimeConvert)) {		
			mSTATE = STATE.IDLE;
			mListener.onAPOutOfSync();
			return;
		}
		
		
		if (mTargetInfo.LastData.compareTo(beacon.SSID)!=0) {
			mTargetInfo.LastData = beacon.SSID;
			mListener.onAPEvent(mTargetInfo.LastData);
		}
		
		// restart a scan
		fastscan();
	}
	
	
	
	/* -----------------------
	 * 
	 * MAIN WIFI DISPATCHER
	 * 
	 * -----------------------
	 */
	
	int FoundWtchDog;
	class WifiScanReceiver extends BroadcastReceiver
	{
		@Override
		public void onReceive(Context context, Intent intent)
		{
			switch (mSTATE)
			{
				case IDLE:
				case UNUSABLE:
					FoundWtchDog = SYNC_OUTBEACON_CNT;
					// nothing to do
					break;
					
				case CAPTURING:
					FoundWtchDog = SYNC_OUTBEACON_CNT;
					onCapturingBeacons(mMainWifiObj.getScanResults());
					break;
					
				case SYNC:	
					FoundWtchDog--;
					for(ScanResult sr : mMainWifiObj.getScanResults()) {
						if (sr.BSSID.compareTo(mTargetInfo.BSSID)==0) {
							onSyncBeacons(sr);
							FoundWtchDog++;
							break;
						}
					}
					if (FoundWtchDog==0) {
						mSTATE = STATE.IDLE;
						mListener.onAPOutOfSync();
					}
					break;						
			}
			
			Log.d(LOG_TAG, "Beacons received ("+mSTATE+")");
		}
	}
	
	
	
	
	
	
	
	/* --------------------------------
	 *
	 *   HELPER TO COMPUTE REGRESSION LINE
	 *   
	 * --------------------------------
	 */
	
	public class LPoint {
		public LPoint(long x, long y) { this.x=x; this.y=y; }
		public long x;
		public long y;
	}
	
	public class FLine {
		public float a;
		public float b;
		public long val(long x) { return (long)(a*(float)x+(float)b); }
	}
	

	protected boolean ComputeLinearRegression(List<SyncInfo> info, FLine line) {
		
		line.a = 0;
		line.b = 0;
		
		if (null == info) return false;
		int sz = info.size();
		if (sz<2)  return false;
		
		// Measurement points
		List<LPoint> M = new ArrayList<LPoint>();
		
		// 1- Compute diff and origin and feed M
		long syslast=0;
		long aplast=0;
		long sysfirst=info.get(0).sysdate;
		long apfirst=info.get(0).apdate;
		Boolean reset = true;
		
		for (int i=0; i<sz; i++) {
			SyncInfo s = info.get(i);

			if (reset) {
				syslast=s.sysdate;
				aplast=s.apdate;
				reset = false;
			
			} else {
				s.sysdiff = s.sysdate - syslast;
				s.apdiff  = s.apdate - aplast;
				syslast=s.sysdate;
				aplast=s.apdate;
							
				/* check */
				float err = Math.abs(s.apdiff - s.sysdiff);
				err = err / (float)s.apdiff;
				if ((err > 0.25f) || (s.apdiff<0) || (s.sysdiff<0))  {
					reset = true;
					Log.e(LOG_TAG, " ap:" + s.apdiff +" sys:" + s.sysdiff + " err:"+err);
				} else {
					Log.i(LOG_TAG, " ap:" + s.apdiff +" sys:" + s.sysdiff);					
					LPoint p = new LPoint(s.sysdate-sysfirst, s.apdate-apfirst);
					M.add(p);
				}
			}
		}
		
		
		// 3- compute regression
		
		int n = M.size(); // number of points
		if (n<2) {
			return false;
		}
		
		
		/* Mean values */
		long sX = 0;
		long sY = 0;		
		for(LPoint p : M) {
			n++;
			sX += p.x;
			sY += p.y;
		}
		float meanX = (float)sX/(float)n;
		float meanY = (float)sY/(float)n;
		
		/* Variances and Covariance*/
		long svX = 0;
		//long svY = 0;
		long sCov = 0;
		for(LPoint p : M) {
			svX += (p.x - meanX) * (p.x - meanX);
			//svY += (p.y - meanY) * (p.y - meanY);
			sCov += (p.x - meanX) * (p.y - meanY);
		}
		float Vx  = (float)svX/(float)n;
		//float Vy  = (float)svY/(float)n;
		float COV = (float)sCov/(float)n;
		
		line.a = COV/Vx;
		line.b = meanY-line.a*meanX;		
		return true;		
	}

	
	
}
