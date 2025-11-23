package org.dru;

import com.fazecast.jSerialComm.SerialPort;
import java.util.function.Consumer;

public class ArduinoController extends Thread {
    private final Consumer<String> callback;
    private SerialPort serialPort;
    private final String portName;

    public ArduinoController(Consumer<String> callback, String portName) {
        this.callback = callback;
        this.portName = portName;
    }

    public void connect() {
        serialPort = SerialPort.getCommPort(portName);
        serialPort.setBaudRate(9600);

        if (serialPort.openPort()) {
            System.out.println("Connesso a " + portName);
        } else {
            System.out.println("Errore: impossibile aprire la porta");
            return;
        }

        new Thread(() -> {
            try {
                while (true) {
                    if (serialPort.bytesAvailable() > 0) {
                        byte[] buffer = new byte[serialPort.bytesAvailable()];
                        serialPort.readBytes(buffer, buffer.length);
                        String data = new String(buffer);
                        System.out.println("Input from Arduino: " + data);
                        callback.accept(new String(buffer));
                    }
                    Thread.sleep(20);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }).start();
    }

    public void takeOff() {
        this.write("TAKE_OFF");
    }
    public void land() {
        this.write("LAND");
    }

    private void write(String msg) {
        if (serialPort != null && serialPort.isOpen()) {
            msg = msg + "\n";
            serialPort.writeBytes(msg.getBytes(), msg.length());
        }
    }

    public void disconnect() {
        if (serialPort != null && serialPort.isOpen()) {
            serialPort.closePort();
        }
    }
}
