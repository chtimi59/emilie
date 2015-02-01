package com.jdodev.updtest;

import java.net.DatagramPacket;
import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import android.util.Log;

public class UDPFrame {
	
	public class TESTFrame {
		public final static int ID=1;
		public int keycode;					
		public byte[] dummy;
		
		public TESTFrame parseb(ByteBuffer b) {
			keycode = byte_signed2unsigned(b.get());
			if (dummy == null) dummy = new byte[1024*10];
			b.get(dummy, 0, 1024*10);
			return this;
		}
	}
	
	public static int  byte_signed2unsigned(byte x)    { return x & 0xFF; }	
	public static int  short_signed2unsigned(short x)  { return x & 0xFFFF; }
	public static long int_signed2unsigned(int x)      { return x & 0xFFFFFFFFl; }

	private boolean mIsCorrupted = false;
	private boolean mIsMisPlaced = false;
	public boolean isValid() { return (!mIsCorrupted) && (!mIsMisPlaced); }
	
	// read header (8bytes)
	public int framenumber;
	public int version;
	public int frametype;
	public long address;					
	public long size;
	public Object payload;
	
	
	public boolean parse(DatagramPacket packet) {
		ByteBuffer b = ByteBuffer.wrap(packet.getData());
		b.order(ByteOrder.LITTLE_ENDIAN);
		
		try {
			// read header (8bytes)
			framenumber = short_signed2unsigned(b.getShort());
			version 	 = byte_signed2unsigned(b.get());
			frametype 	 = byte_signed2unsigned(b.get());
			address 	 = int_signed2unsigned(b.getInt());					
			size 	 	 = int_signed2unsigned(b.getInt());
		
			switch(frametype)
			{
				case TESTFrame.ID:
					TESTFrame tstfrm = new TESTFrame();
					payload = tstfrm.parseb(b);
					break;
			}

			return true;
			
		} catch(BufferUnderflowException e) {
			Log.e(UDPClient.TAG, e.toString());
			mIsCorrupted = true;
			received_corrupted++;
			return false;
		}		
	}

	
	
	static long received = 0;
	static long missing = 0;
	// total = received + missing 

	static long received_ok = 0;
	static long received_corrupted = 0;
	static long received_misplaced = 0;
	// received = received_ok + received_corrupted + received_misplaced
	
	// following value are not reseted on rollover
	static long stats_total_inbytes = 0;
	static long stats_total_received = 0;
	
	public static void ClearStats() {
		ClearStatsRollover();
		stats_total_inbytes = 0;
		stats_total_received = 0;
	}
	public static void ClearStatsRollover() {
		received = 0;
		missing = 0;
		received_ok = 0;
		received_corrupted = 0;
		received_misplaced = 0;
	}
	
	public void computestats(long old_framenumber) {
		if (framenumber<=old_framenumber) mIsMisPlaced = true;
		
		received++;
		missing += framenumber-old_framenumber-1;
		
		if (mIsMisPlaced) received_misplaced++;
		//if (mIsCorrupted) received_corrupted++;
		if (isValid()) received_ok++;
		
		stats_total_inbytes += size + 8;
		stats_total_received++;
	}

	
	public String toString() {
		
		String text = 
			"frame ["+framenumber + "]\n" +
			"" + String.format("%.02f", (float)(stats_total_inbytes)/1024.0f/1024.0f) + "MB\n" +
			"Quality " + String.format("%.02f", 100*(float)(received_ok)/(float)(received+missing)) + "%\n" +
			"correct: "   + received_ok + "\n" +
			"missing: "   + missing + "\n" +
			"corrupted: " + received_corrupted + "\n" +
			"misplaced: " + received_misplaced + "\n";
		
		switch(frametype) {
			case 1: // TEST_FRAME_ID
				TESTFrame tstfrm = (TESTFrame)payload;
				if (tstfrm.keycode!=0) {
					text += "Key pressed " + String.format("0x%02X", tstfrm.keycode);
				}
				break;
		}
		
		return text;
	}
	
	

}
