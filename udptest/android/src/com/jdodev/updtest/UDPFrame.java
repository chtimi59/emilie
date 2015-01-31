package com.jdodev.updtest;

import java.net.DatagramPacket;
import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import android.util.Log;

public class UDPFrame {
	
	public class TESTFrame {
		public long keycode;					
		public byte[] dummy;
	}
	
	public static int  byte_signed2unsigned(byte x)    { return x & 0xFF; }	
	public static int  short_signed2unsigned(short x)  { return x & 0xFFFF; }
	public static long int_signed2unsigned(int x)     { return x & 0xFFFFFFFF; }

	
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
		
			switch(frametype) {
				case 1: // TEST_FRAME_ID
					TESTFrame tstfrm = new TESTFrame();
					tstfrm.keycode = byte_signed2unsigned(b.get());
					b.get(tstfrm.dummy, 0, 1024*10);
					payload = tstfrm; 
					break;
			}
			
			return true;
			
		} catch(BufferUnderflowException e) {
			Log.e("ERR",e.toString());
			return false;
		}		
	}

	
	
	long stats_missing = 0;
	long stats_received = 0;
	long stats_total = 0;
	long stats_total_bytes = 0;
	
	public void computestats(long old_framenumber) {
		stats_missing += framenumber-old_framenumber-1;
		stats_received++;
		stats_total=stats_received+stats_missing;
		stats_total_bytes += size + 8;
		
	}
	
	public String toString() {
		
		String text = 
			"frame No ["+framenumber + "]\n" +
			"" + String.format("%.02f", (float)(stats_total_bytes/1024)) + "kB\n" +
			"Quality " + String.format("%.02f", 100*(float)(stats_received)/(float)(stats_total)) + "%\n" +
			"received: " + stats_received + "\n" +
			"missing: " + stats_missing + "\n";
		
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
