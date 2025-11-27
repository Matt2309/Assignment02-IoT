package org.dru;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.image.Image;
import javafx.scene.layout.*;
import javafx.stage.Stage;

public class App extends Application {

    private final Label lblStatoDrone = new Label("Stato Drone: -");
    private final Label lblStatoHangar = new Label("Stato Hangar: -");
    private final Label lblDistanza = new Label("Distanza: -");
    private final Label outArduino = new Label("");
    private final Led ledHangar = new Led(10);

    private final TextArea console = new TextArea();
    ArduinoController controller = new ArduinoController(this::onSerialDataReceived);

    // Callback da Arduino
    private String statoDroneLogico = "riposo";
    private String statoHangarLogico = "normale";

    private void onSerialDataReceived(String data) {

        if (data == null) return;
        final String raw = data;

        Platform.runLater(() -> {
            String d = raw.replace("\r","").replace("\n","").trim();
            if (d.isEmpty()) return;

            if (d.contains("MSG")) {
                outArduino.setText("From arduino: " + d);

                //MAPPATURA STATI DRONE
                if (d.contains("DRONE_INSIDE")) {
                    statoDroneLogico = "riposo";
                    statoHangarLogico = "normale";
                    ledHangar.setOff();
                }

                if (d.contains("TAKE_OFF")) {
                    statoDroneLogico = "decollo";
                    ledHangar.setOff();
                }

                if (d.contains("DRONE_OUT")) {
                    statoDroneLogico = "funzionamento";
                    statoHangarLogico = "normale";
                    ledHangar.setOff();
                }

                if (d.contains("LANDING")) {
                    statoDroneLogico = "atterraggio";
                    statoHangarLogico = "normale";
                    ledHangar.setOff();
                }

                if(d.contains("ERROR")){
                    appendLog(d);
                }

                //MAPPATURA HANGAR
                if (d.contains("ALARM") && !d.contains("ERROR")) {
                    statoHangarLogico = "ALLARME";
                    ledHangar.setRed();
                }

                if (d.contains("PRE_ALARM")) {
                    statoHangarLogico = "normale (PRE-ALLARME)";
                    ledHangar.setYellow();
                }
                lblDistanza.setVisible(false);
                lblStatoHangar.setText("Stato hangar: " + statoHangarLogico);
                lblStatoDrone.setText("Stato drone: " + statoDroneLogico);

                //DISTANZA
                if (d.contains("DIST")) {
                    if (statoDroneLogico.equals("atterraggio")) {
                        String val = d.split(" ")[2];
                        lblDistanza.setText("Distanza: " + val + " cm");
                        lblDistanza.setVisible(true);
                    }
                }
            }
        });
    }

    //UI
    @Override
    public void start(Stage stage) {

        //Sezione Superiore - Porta + Connessione
        ComboBox<String> portSelector = new ComboBox<>();
        for (String p : controller.getAvailablePorts())
            portSelector.getItems().add(p);
        if (!portSelector.getItems().isEmpty())
            portSelector.getSelectionModel().selectFirst();

        Button connectBtn = new Button("Connetti");
        Button disconnectBtn = new Button("Disconnetti");

        HBox topBar = new HBox(10, new Label("Porta:"), portSelector, connectBtn, disconnectBtn);
        topBar.setAlignment(Pos.CENTER_LEFT);
        topBar.setPadding(new Insets(10));
        topBar.setStyle("-fx-background-color: rgba(0,0,0,0.35); -fx-background-radius:10;");


        //Pulsanti di comando drone (orizzontali)
        Button takeOffBtn = new Button("Decollo");
        Button landBtn = new Button("Atterraggio");
        Button resetBtn = new Button("RESET");

        HBox commandBar = new HBox(20, takeOffBtn, landBtn, resetBtn);
        commandBar.setAlignment(Pos.CENTER);
        commandBar.setPadding(new Insets(15));
        commandBar.setStyle("-fx-background-color: rgba(0,0,0,0.35); -fx-background-radius:10;");

        HBox hangarStatusLine = new HBox(10, lblStatoHangar, ledHangar);
        hangarStatusLine.setAlignment(Pos.CENTER_LEFT);


        //Sezione Stato
        VBox statusBox = new VBox(10,
                lblStatoDrone,
                hangarStatusLine,
                lblDistanza,
                outArduino
        );
        statusBox.setPadding(new Insets(10));
        statusBox.setStyle("-fx-font-size: 16px; -fx-text-fill: white");
        statusBox.setAlignment(Pos.CENTER_LEFT);


        //Console
        console.setEditable(false);
        console.setPrefRowCount(10);
        console.setStyle("-fx-control-inner-background: black; -fx-text-fill: #00FF00;");

        VBox centerContent = new VBox(15, statusBox, console);
        centerContent.setPadding(new Insets(10));
        centerContent.setStyle("-fx-background-color: rgba(0,0,0,0.5); -fx-background-radius:10;");


        //Contenitore principale
        VBox root = new VBox(20, topBar, commandBar, centerContent);
        root.setPadding(new Insets(20));

        //Sfondo elegante
        Image img = new Image(getClass().getResource("/img/hangar.jpg").toExternalForm());
        BackgroundImage bgImg = new BackgroundImage(
                img,
                BackgroundRepeat.NO_REPEAT, BackgroundRepeat.NO_REPEAT,
                BackgroundPosition.CENTER,
                new BackgroundSize(BackgroundSize.AUTO, BackgroundSize.AUTO, false, false, true, true)
        );
        root.setBackground(new Background(bgImg));


        //Eventi Pulsanti
        connectBtn.setOnAction(e -> {
            String port = portSelector.getValue();
            if (port == null) {
                appendLog("Nessuna porta selezionata");
                return;
            }

            controller.connect(port);
            appendLog("Connesso a " + port);
        });

        disconnectBtn.setOnAction(e -> {
            if (controller != null) controller.disconnect();
            appendLog("Disconnesso");
        });

        takeOffBtn.setOnAction(e -> {
            if (controller != null) controller.takeOff();
            appendLog("→ TAKE_OFF");
        });

        landBtn.setOnAction(e -> {
            if (controller != null) controller.land();
            appendLog("→ LAND");
        });

        resetBtn.setOnAction(e -> {
            if (controller != null) controller.Reset();
            appendLog("→ RESET");
        });


        Scene scene = new Scene(root, 900, 600);
        scene.getStylesheets().add(getClass().getResource("/style.css").toExternalForm());
        stage.setTitle("DRU System - Interfaccia Hangar Drone");
        stage.setScene(scene);
        stage.show();
    }


    // Aggiunta log thread-safe
    private void appendLog(String s) {
        if (Platform.isFxApplicationThread()) {
            console.appendText(s + "\n");
        } else {
            Platform.runLater(() -> console.appendText(s + "\n"));
        }
    }

    public static void main(String[] args) {
        launch(args);
    }
}
