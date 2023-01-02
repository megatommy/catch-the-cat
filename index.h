const char PAGE_INDEX[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="icon" href="data:,">
    <title>😸 Catch the Cat</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        .p-15 {
            padding: 15px;
        }

        .m-15 {
            margin: 15px;
        }

        .d-none {
            display: none;
        }

        .border-black {
            border: 2px solid black;
        }

        .color-red {
            color: #d00000;
        }

        .heading {
            font-size: 1.3em;
        }

        .btn {
            padding: 7px;
            font-size: 1.3rem;
            cursor: pointer;
        }

        body {
            font-size: 1.3rem;
            font-family: sans-serif;
            color: #222;
            background-color: #ffe8d6;
        }

        .container {
            width: 100%;
            max-width: 960px;
            margin: 0 auto;
        }

        .top-bar {
            display: flex;
            justify-content: space-between;
            align-items: center;
            background-color: #cb997e;
        }

        .distance-sensor-text {
            text-align: center;
        }

        .distance-sensor-value {
            display: flex;
            justify-content: center;
            align-items: baseline;
        }

        .distance-sensor-value #distance_sensor_value {
            font-size: 6.5rem;
        }

        .distance-sensor-value .unit {
            font-size: 2rem;
        }

        #threshold_cm, #check_every, #readings_to_check {
            padding: 10px;
            font-size: 1rem;
            width: 80px;
            margin: 0 5px;
        }

        .contact-details input[type="checkbox"] {
            width: 16px;
            height: 16px;
        }

        .contact-details input {
            max-width: 400px;
            width: 100%;

        }


        .form-row {
            margin: 15px 0;
        }

        .form-row input, .form-row select {
            padding: 10px;
            font-size: 1rem;
        }



        .save-btn {
            width: 100%;
            border: 1px solid #386641;
            background-color: #6a994e;
            color: white;
        }

    </style>
</head>
<body>
    <div class="container">
        <div class="top-bar p-15">
            <div class="heading">😸 Catch the Cat</div>
            <!-- <button onclick="test_email();">get data</button> -->
            <h3 id="uptime">00:00:00</h3>
        </div>

        <div class="distance-sensor-info p-15">
            <div class="distance-sensor-text">Valore sensore di distanza:</div>
            <div class="distance-sensor-value">
                <div id="distance_sensor_value">00.0</div>
                <div class="unit">cm</div>
            </div>
        </div>
        
        <form action="/save-data" method="POST" id="form" onsubmit="return submitForm();">
            <div class="border-black p-15 m-15">
                <div class="form-row">
                    Controlla ogni
                    <input type="number" name="check_every" id="check_every" placeholder="1" required> secondi che il valore sia
                    <select onchange="checkThreshold();" name="than" id="than" required>
                        <option value="less-than">minore di </option>
                        <option value="greater-than">maggiore di</option>
                    </select>
                    <input onkeyup="checkThreshold();" type="number" name="threshold_cm" id="threshold_cm" placeholder="40" required>
                    <span>cm</span>
                    per
                    <input type="number" name="readings_to_check" id="readings_to_check" placeholder="3" required> volte.
                </div>
            </div>

            <div class="contact-details border-black p-15 m-15">
                <div class="heading">Impostazioni per SMS</div>
                <div class="contact-info">
                    <div class="form-row">
                        <label for="sim_pin">Codice PIN della SIM</label><br />
                        <input type="number" name="sim_pin" id="sim_pin" placeholder="1234" />
                    </div>

                    <div class="form-row">
                        <label for="sms_number">Numero</label><br />
                        <input type="text" name="sms_number" id="sms_number" placeholder="+39xxxxxxxxxx" />
                    </div>

                    <div class="form-row">
                        <label for="sms_message">Messaggio</label><br />
                        <input type="text" name="sms_message" id="sms_message" maxlength="120" placeholder="Gatto preso!" />
                    </div>

                    <div class="form-row">
                        <button type="button" onclick="testSMS();" id="test_sms" class="btn">💬 Prova SMS</button>
                    </div>
                </div>
            </div>

            <div class="m-15">
                <button type="submit" class="btn save-btn" id="save_btn">Completa setup</button>
            </div>
        </form>
    </div>



    <script>
        let last_response = null;
        let data_sent = false;
        let ls = window.localStorage;

        window.addEventListener('DOMContentLoaded', function(event) {
            getData();
        });

        function $(selector){
            let el = document.querySelectorAll(selector);
            return el.length > 1 ? el : el[0];
        }

        function getData(){
            fetch('/get-data').then(response => response.text())
            .then(function(response){
                let data = response.split('---');

                last_response = {
                    uptime: data[0],
                    sensor_value: data[1]
                };
                $('#uptime').innerText = last_response.uptime;
                $('#distance_sensor_value').innerText = last_response.sensor_value;
                
                checkThreshold();

                setTimeout(getData, 2000);
            });
            
        }

        function checkThreshold(){
            if(!last_response){
                // return;
            }
            let threshold_cm = parseFloat($('#threshold_cm').value);
            let than = $("#than").value;

            if(
                (threshold_cm < last_response.sensor_value && than == 'less-than')
                || (threshold_cm > last_response.sensor_value && than == 'greater-than')
            ) {
                $("#distance_sensor_value").classList.remove("color-red");
            } else {
                $("#distance_sensor_value").classList.add("color-red");
            }
        }

        function testSMS(){
            if($("#sms_number").value ==""){
                alert("❌ Primo numero vuoto");
                return;
            }
            
            $("#test_sms").disabled = true;
            $("#test_sms").innerText = 'Prova in corso...';
            let data = {
                sim_pin: $('#sim_pin').value,
                sms_number: $('#sms_number').value,
                sms_message: ($('#sms_message').value != "" ? $('#sms_message').value : 'SMS di prova')
            };
            fetch('/test-sms', {
                method: 'POST',
                body: new URLSearchParams(data)
            }).then(response => response.text())
            .then(function(response) {
                $("#test_sms").disabled = false;
                if(response == 1){
                    alert("✅ SMS mandato.");
                } else {
                    alert('❌ Errore. Controlla i dati e riprova.');
                }

                $("#test_sms").disabled = false;
                $("#test_sms").innerText = '💬 Prova SMS';
            });
        }

        function submitForm(){
            if($("#distance_sensor_value").classList.contains("color-red")){
                alert("⚠️ Con i valori attualmente misurati, scatterebbe sibuto.");
                return false;
            }
            return confirm("Procedere al salvataggio?");
        }
        
    </script>
</body>
</html>
)=====";