package org.dru;

import com.fazecast.jSerialComm.SerialPort;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

public class ArduinoController extends Thread {
    private final Consumer<String> callback;
    private SerialPort serialPort;

    public ArduinoController(Consumer<String> callback) {
        this.callback = callback;
    }

    public List<String> getAvailablePorts() {
        List<String> result = new ArrayList<>();

        for (SerialPort port : SerialPort.getCommPorts()) {
            String systemName = port.getSystemPortName();
            String description = port.getDescriptivePortName();

            result.add(systemName + " (" + description + ")");
        }

        return result;
    }

    public void connect(String portName) {
        String systemPortName = "/dev/"+portName.split(" ")[0];
        System.out.println("Connecting to port " + systemPortName);
        this.serialPort = SerialPort.getCommPort(systemPortName);
        serialPort.setComPortParameters (9600 , Byte.SIZE , SerialPort.ONE_STOP_BIT, SerialPort.NO_PARITY ) ;
        serialPort.setComPortTimeouts (SerialPort.TIMEOUT_WRITE_BLOCKING,0 ,0);
        if (serialPort.openPort()) {
            System.out.println("Porta aperta: " + systemPortName);
        } else {
            System.out.println("Errore apertura porta");
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
        this.write("TAKEOFF");
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

    public void Reset() {
        this.write("RESET");
    }
}
