package com.jdodev.emilie;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Path.FillType;
import android.graphics.Point;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

public class MyGraph extends View {

	Paint mPaint;
    
	public MyGraph(Context context) {
		super(context);
		mPaint = new Paint();
		setWillNotDraw(false);
		Clear();
	}
	public MyGraph(Context context, AttributeSet attrs) {
		super(context, attrs);
		mPaint = new Paint();
		setWillNotDraw(false);
		Clear();
	}
	public MyGraph(Context context, AttributeSet attrs, int defStyleAttr) {
		super(context, attrs, defStyleAttr);
		mPaint = new Paint();
		setWillNotDraw(false);
		Clear();
	}

	
	
	Path mPath = null;
	final int STATS=30;
	Point[] P = null;
    int idx;
    long x0;
    long y0;
    Point min;
	Point max;

	public void Clear() {
		mPath = null;
		P = new Point[STATS];
	    idx = -1;
	}
	
	public void addMeasurement(long x, long y) {

		if (idx==-1) {
			x0 = x;
			y0 = y;
			for (int i=0;i<STATS;i++) {
				P[i] = new Point();
				P[i].x = (int)(x-x0);
				P[i].y = (int)(y-y0);
			}
			idx = 0;
			return;			
		}
		
		P[idx].x = (int)(x-x0);
		P[idx].y = (int)(y-y0);
		idx = (idx+1)%STATS;		
		
		min = new Point();
		max = new Point();
		for (int i=0;i<STATS;i++) {
			if (i==0) {
				min.x = P[i].x;
				min.y = P[i].y;
				max.x = P[i].x;
				max.y = P[i].y;
			} else {
				if (P[i].x < min.x) min.x = P[i].x;
				if (P[i].y < min.y) min.y = P[i].y;
				if (P[i].x > max.x) max.x = P[i].x;
				if (P[i].y > max.y) max.y = P[i].y;
			}
		}
		
		// padding...
		max.x += 10;
		max.y += 10;
		min.x -= 10;
		min.y -= 10;
		
		
		
		int W = this.getWidth();
		int H = this.getHeight();
		
		mPath = new Path();
		mPath.setFillType(FillType.EVEN_ODD);
		for (int i=0;i<STATS;i++) {
		    float X = W * (float)(P[i].x - min.x) / (float)(max.x-min.x);
		    float Y = H * (float)(P[i].y - min.y) / (float)(max.y-min.y);
			if (i==0) {
				mPath.moveTo(X,Y);
			} else {
				mPath.lineTo(X,Y);
			}		    		    
	    }
		 
		Log.d("STAT","DRAW GRAPH! X:" + ((max.x-min.x)/1000L) + "s Y:" + ((max.y-min.y)/1000L) + "s");
		this.invalidate();
	}
	
	@Override
	protected void onDraw(Canvas canvas) {
		 super.onDraw(canvas);
		 if (mPath==null) return;
		
		 int W = this.getWidth();
		 int H = this.getHeight();
		 mPaint.setColor(Color.parseColor("#000055"));
		 canvas.drawRect(0, 0, W, H, mPaint);
		 
		 mPaint.setColor(Color.parseColor("#FFFFFF"));
		 mPaint.setStrokeWidth(4);
		 mPaint.setStyle(Paint.Style.FILL_AND_STROKE);
		 mPaint.setAntiAlias(true);
	     canvas.drawPath(mPath, mPaint);		    		 
	}
	
}
