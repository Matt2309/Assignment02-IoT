package org.dru;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.scene.Scene;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextArea;

public class App extends Application {
    private final Label outArduino = new Label("");

    private void onSerialDataReceived(String data) {
        Platform.runLater(() -> outArduino.setText(data));
    }
    @Override
    public void start(Stage stage) {
        String portName = "COM4";
        ArduinoController controller = new ArduinoController(this::onSerialDataReceived, portName)

        TextArea console = new TextArea();
        console.setEditable(false);

        Button connectBtn = new Button("Connetti a COM4");
        Button disconnectBtn = new Button("Disconnetti da COM4");
        Button ledOnBtn = new Button("LED ON");
        Button ledOffBtn = new Button("LED OFF");

        // Connection
        connectBtn.setOnAction(e -> {
            controller.connect();
        });
        disconnectBtn.setOnAction(e -> {
            controller.disconnect();
        });

        /*
        ledOnBtn.setOnAction(e -> controller.write("LED_ON"));
        ledOffBtn.setOnAction(e -> controller.write("LED_OFF"));
        */
        VBox root = new VBox(10,
                connectBtn,
                disconnectBtn,
                ledOnBtn, 
                ledOffBtn,
                outArduino,
                console
        );

        stage.setScene(new Scene(root, 400, 400));
        stage.setTitle("JavaFX ↔ Arduino (COM4)");
        stage.show();
    }

    public static void main(String[] args) {
        launch(args);
    }
}
