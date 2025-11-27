package org.dru;

import javafx.scene.paint.Color;
import javafx.scene.shape.Circle;

public class Led extends Circle {

    public Led(double radius) {
        super(radius);
        setOff();
    }

    public void setRed() {
        setFill(Color.RED);
        setStroke(Color.ORANGERED);
    }

    public void setYellow() {
        setFill(Color.YELLOW);
        setStroke(Color.GOLDENROD);
    }

    public void setOff() {
        setFill(Color.GREEN);
        setStroke(Color.BLACK);
    }
}
