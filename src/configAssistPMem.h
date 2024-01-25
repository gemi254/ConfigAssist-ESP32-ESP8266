// Minimal application config dictionary
const char* CA_DEFAULT_DICT_JSON PROGMEM = R"=====(
Wifi settings:
  - st_ssid:
      label: Name for WLAN
      default:
  - st_pass:
      label: Password for WLAN
      default:
  - host_name: 
      label: >-
        Host name to use for MDNS and AP
        {mac} will be replaced with device's mac id
      default: configAssist_{mac}
)=====";
// Template for message page
PROGMEM const char CONFIGASSIST_HTML_START[] = R"=====(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
  <meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no"/>
  <link rel="shortcut icon" href="#" />
  <title>{title}</title>                        
</head>
)=====";

// Template for message page
PROGMEM const char CONFIGASSIST_HTML_MESSAGE[] = R"=====(
  <script>
  document.addEventListener('DOMContentLoaded', function (event) {
      setTimeout(function() {
                location.href = '{url}';
            }, {refresh});
      
      if({reboot}){      
        const baseHost = document.location.origin;
        const url = baseHost + "/cfg?_RBT_CONFIRM=1";
        try{
          console.log('Restarting')
          const response = fetch(encodeURI(url));
        } catch (e) {
          console.log(e)
        }
      }
  });
  </script>                  
</head>
<body>
<div style="text-align:center;"><h3>{msg}</h3></div>
</body></html>
)=====";

// Template for uploading a file
PROGMEM const char CONFIGASSIST_HTML_UPLOAD[] = R"=====(
<body>
<div style="text-align:center;"><h3>Select a config file to upload to the device</h3></div>
<div style="text-align:center;">
  <form action='/fupl' method='post' enctype='multipart/form-data'>
  <input class='buttons' style='width:40%' type='file' name='fupload' id='fupload' value=''><br>
  <br><button class='buttons' style='width:10%' type='submit'>Upload File</button><br><br>
  [&nbsp;<a href='/cfg'>Back</a>&nbsp;]<br><br>
</div>
</body>
</html>
)=====";
#ifdef CA_USE_OTAUPLOAD
// Template for uploading a ota file
const char* CONFIGASSIST_HTML_OTAUPLOAD = R"=====(
<script>
  const port = !window.location.port ? "80" : window.location.port;    
  const otaServer = 'http://' + document.location.hostname + ':' + port;
  const $ = document.querySelector.bind(document);
  
  async function uploadFile() {
    // submit file for uploading
    let file = $("#fupload").files[0];
    let formdata = new FormData();
    formdata.append("fupload", file);
    let ajax = new XMLHttpRequest();
    ajax.upload.addEventListener("progress", progressHandler, false);
    ajax.addEventListener("load", completeHandler, false);
    ajax.addEventListener("error", errorHandler, false);
    ajax.addEventListener("abort", abortHandler, false);
    ajax.open("POST", otaServer + '/fupl?ota=1');
    ajax.send(formdata);
  }

  function progressHandler(event) {
    $("#bytes_uploaded").innerHTML = "Uploaded " + event.loaded + " of " + event.total + " bytes";
    let percent = (event.loaded / event.total) * 100;
    $("#progressBar").value = Math.round(percent);
    $("#upl_status").innerHTML = Math.round(percent) + "% transferred";
    if (event.loaded  == event.total) $("#upl_status").innerHTML = 'Uploaded, wait for completion result';
  }

  function completeHandler(event) {
    $("#upl_status").innerHTML = event.target.responseText;
    const url = document.location.origin + "/cfg?_RBT_CONFIRM=1";
    setTimeout(()=> {
      try{
        console.log('Restarting')
        const response = fetch(encodeURI(url));
      } catch (e) {
        console.log(e)
      }
    }
    ,500);
    
    setTimeout(()=> {
      location.href =  document.location.origin + "/cfg";
    }
    ,8000);
    
  }

  function errorHandler(event) {
    $("#upl_status").innerHTML = "Upload Failed";
  }

  function abortHandler(event) {
    $("#upl_status").innerHTML = "Upload Aborted";
  }
</script>
<body>
  <div style="text-align:center;"><h3>Select a firmware to upload to the device</h3></div>
  <div style="text-align:center;">
    <form id="upload_form" enctype="multipart/form-data" method="post">
      <progress id="progressBar" value="0" max="100" style="width:300px; height:25px;"></progress>
      <br></br>
      <input type="file" name='fupload' id='fupload' onchange="uploadFile()"><br>
      <h3 id="upl_status"></h3>
      <p id="bytes_uploaded"></p>
      [&nbsp;<a href='/cfg'>Back</a>&nbsp;]<br><br>
    </form>
  </div>  
</body>
</html>
)=====";
#endif

#ifdef CA_USE_FIMRMCHECK
// Template for checking firmware
PROGMEM const char CONFIGASSIST_HTML_FIRMW_CHECK[] = R"=====(
<script>
const port = !window.location.port ? "80" : window.location.port;    
  const otaServer = 'http://' + document.location.hostname + ':' + port;
  const $ = document.querySelector.bind(document);
  var otaUrl = "";

  async function uploadFileFromUrl() {
    // submit file for uploading    
    if(otaUrl == "" ) return;
    fetch(otaUrl)
        .then(r => r.blob())
        .then(r => {
            console.log(r)
            var filename = otaUrl.replace(/^.*[\\/]/, '')
            let file = r;
            let formdata = new FormData();
            formdata.append("filename", filename);
            formdata.append("fupload", file);

            let ajax = new XMLHttpRequest();
            ajax.upload.addEventListener("progress", progressHandler, false);
            ajax.addEventListener("load", completeHandler, false);
            ajax.addEventListener("error", errorHandler, false);
            ajax.addEventListener("abort", abortHandler, false);
            ajax.open("POST", otaServer + '/fupl?ota=1');
            ajax.send(formdata);
        })
    
  }

  function progressHandler(event) {
    $("#bytes_uploaded").innerHTML = "Uploaded " + event.loaded + " of " + event.total + " bytes";
    let percent = (event.loaded / event.total) * 100;
    $("#progressBar").value = Math.round(percent);
    $("#upl_status").innerHTML = Math.round(percent) + "% transferred";
    if (event.loaded  == event.total) $("#upl_status").innerHTML = 'Uploaded, wait for completion result';
  }

  function completeHandler(event) {
    $("#upl_status").innerHTML = event.target.responseText;
    const url = document.location.origin + "/cfg?_RBT_CONFIRM=1";
    setTimeout(()=> {
      try{
        console.log('Restarting')
        const response = fetch(encodeURI(url));
      } catch (e) {
        console.log(e)
      }
    }
    ,500);
    
    setTimeout(()=> {
      location.href =  document.location.origin + "/cfg";
    }
    ,8000);
    
  }

  function errorHandler(event) {
    $("#upl_status").innerHTML = "Upload Failed";
  }

  function abortHandler(event) {
    $("#upl_status").innerHTML = "Upload Aborted";
  }

  function checKFirmware(url){
    if(url==""){
      $("#firmwareInfo").innerHTML += "No firmware OTA url defined in configs"      
      return;
    } 
    fetch(url).then((r) => {
        if (r.ok) {
            return r.json();
        }else if(r.status = 404){
            $("#firmwareInfo").innerHTML = "No firmware info file found!"
            $("#firmwareLink").innerHTML = "Url: " + url;
        }else{
            console.log(r.statusText);
        }        
    })
    .then(r => {
      console.log(r)
      if(r=="") return;      
      
      otaUrl = r.url;
      var filename = r.url.replace(/^.*[\\/]/, '')
      if(r.ver!="" && $("#fwv").innerHTML != r.ver){
        $("#firmwareInfo").innerHTML = "New firmware version found: <font style='color: blue;'>" + r.ver +"</font>"
        $("#firmwareLink").innerHTML += "File: " + "<font style='color: blue;'>" + filename + "</font>"
        $("#firmwareDescr").innerHTML += r.descr + ""
        $("#upgrade").style.display = "";
        $("#progressBar").style.display = "";
      }else{
        $("#firmwareInfo").innerHTML = "You have the latest firmware version"
        $("#upgrade").style.display = "";
        $("#progressBar").style.display = "none";
      }
    })
    .catch(e => {
      console.log('Url error', e)  
    })
  }
// Firmware check
checKFirmware("{FIRMWARE_URL}")
</script>
<body>
  <div style="text-align:center;">
    <h3>Your current firmware version is: <span style="color: gray;" id="fwv">{FIRMWARE_VERSION}</span></b></h3>
  </div>
  <div style="text-align:center;">
      <h3 id="firmwareInfo"></h3>
      <h4 id="firmwareLink"></h4>
      <h4 id="firmwareDescr"></h4>  
      <button style="display:none;" name='fupload' id='upgrade' onclick="uploadFileFromUrl()">Upgrade firmware</button>
      <br><br>
      <progress id="progressBar" style="display:none;" value="0" max="100" style="width:300px; height:25px;"></progress>
      <h4 id="upl_status"></h4>      
      <p id="bytes_uploaded"></p>
      <br>
      [&nbsp;<a href='/cfg'>Back</a>&nbsp;]<br><br>
  </div>  
</body>
</html>
)=====";
#endif

// Template for header, begin of the config form
PROGMEM const char CONFIGASSIST_HTML_CSS[] =R"=====(
<style>
:root {
  --bg-table-stripe: #f6f6f5;
  --b-table: #999a9b47;
  --caption: darkgray;  
}
body {
  font-family: Arial, Helvetica, sans-serif;
}
.column {
  float: center;
  width: 100%;
  padding: 0 10px;
  box-sizing: border-box;
  margin-top: 10px;
}
.row {
  margin: 0 -5px;
}
.row:after {
  content: "";
  display: table;
  clear: both;
}
.column .card.closed {
  height: 26px;
  overflow: hidden;
}
.column .card.closed h3 {
  color:  darkgray;
}
.card {
  border: 1px solid #cbcaca;
  box-shadow: 0 2px 4px 0 rgba(0, 0, 0, 0.2);
  padding: 10px;
  text-align: center;
  background-color: #ffffff;
  margin-bottom: 8px;
  border-radius: 5px;
}
.card h3,
.card .toggle h3{
  margin:2px;
  color: darkslategray
  background-color: whitesmoke;
}
.card h3:hover{
  color: #3f51b5;
  cursor: pointer;
  background-color: cornsilk;
}
.card h2{
  margin-top:2px;
  margin-bottom:8px;
  color: var(--caption);
}
.container {
  display: flex;
  justify-content: center;
  flex-direction: row;
}
table {
  width: 100%;
}
table th, table td {
  padding: .325em;
  border: 1px solid var(--b-table);
}
tbody tr:nth-of-type(2n+1) {
  background-color: var(--bg-table-stripe)
}
.card-header{
  margin-bottom: 15px;
}
.card-header:hover {
  background-color: #f7fafa;
}
.card-body {  
}
.card-key {
  text-align: right;
  font-weight: 800;
  width: 25%;
}
.card-val {
  text-align: left;
  width: 25%;
}
.card-val-ctrl{
  position: relative;
  display: flex;
  flex-wrap: nowrap;
  line-height: var(--elmDbl);
  margin-top: var(--elmHalf);
}
.card-lbl {
  text-align: left;
  font-style: italic;
  font-size: .8em;
  max-width: 450px;
  overflow-wrap: anywhere;
}
button {
  background-color: #fff;
  border: 1px solid #d5d9d9;
  border-radius: 4px;
  box-shadow: rgb(213 217 217 / 50%) 0 2px 5px 0;
  box-sizing: border-box;
  font-weight: 900;
  color: #0f1111;
  cursor: pointer;
  display: inline-block;
  padding: 0 10px 0 11px;
  margin: 2px;
  line-height: 29px;
  position: relative;
  text-align: center;
  vertical-align: middle;
}
button:hover {
  background-color: #f7fafa;
}

.accordBtt {
    background-image:url('data:image/svg+xml,\00003csvg focusable="false" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">\00003cpath fill="%2370757a" d="M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z">\00003c/path>\00003c/svg>');    
    display: inline-block;
    height: 28px;
    width: 28px;
    position: relative; 
    float: right; 
    margin-top: -34px;
    margin-right: 30px;
    cursor: pointer;
        font-size: medium;
    font-weight: normal;
}

.accordBtt.closed{
    transform: rotateZ(0deg);
    transition: transform 200ms ease-in-out;    
}
.accordBtt.open{
    transform: rotateZ(180deg); 
    transition: transform 200ms ease-in-out;    
}

/* Responsive columns */
@media screen and (max-width: 600px) {
  .column {
    width: 100%;
    display: block;
    margin-bottom: 20px;
    margin-top: 2px;
  }
  table {
    border: 0;
  }
  table tr {
    display: block;
    margin-bottom: 0.125em;
  }
  table td {
    display: block;
    font-size: 0.8em;
    text-align: right;
  }
  table td:last-child {
    border-bottom: 0;
  }  
  .card-key{
    text-align: left;
    width:auto; 
  }  
  .card-val{
    width:auto; 
  }
}
</style>)=====";
// Html controls
PROGMEM const char CONFIGASSIST_HTML_CSS_CTRLS[] = R"=====(
<style>
:root {
/* colors used on web page - see https://www.w3schools.com/colors/colors_names.asp */
--toggleActive: lightslategrey;
--toggleInactive: lightgray;
--sliderBodyBackground: darkgray;
--sliderText: black;
--sliderThumb: lightgray;
--sliderBackground: WhiteSmoke; 

/* element sizes, 1rem represents the font size of the root element */
--elmSize: 1rem;
--elmHalf: calc(var(--elmSize) / 2);
--elmQuart: calc(var(--elmSize) / 4);
--elmDbl: calc(var(--elmSize) * 2);
--inputHeight: calc(var(--elmSize) * 1.5);
}
input {
  min-width: calc(var(--elmSize) * 11);
  height: var(--inputHeight);
  border-radius: var(--elmQuart);
  border: 1px solid gray;
  width: 99%
}

textarea {
  min-width: calc(var(--elmSize) * 11);      
  border-radius: var(--elmQuart);        
  border: 1px solid gray;
  width: 99%
}

input[type=range] {
  -webkit-appearance: none;
  width: 99%;
  height: calc(var(--elmHalf) * 3/4);
  background: var(--sliderBodyBackground);
  cursor: pointer;
  margin-top: calc(var(--elmSize) * 3/4);
  min-width: calc(var(--elmSize) * 8);
  border: 1px solid lightgray;
}

input[type=range]:focus {
  outline: 0;
  border: 1px solid gray;
}

input[type=range]::-webkit-slider-runnable-track {
  width: 99%;
  height: 2px;
  cursor: pointer;
  background: var(--sliderBackground); 
  border-radius: var(--elmHalf);
}

input[type=range]::-webkit-slider-thumb {
  height: calc(var(--elmSize) * 1.1);
  width: calc(var(--elmSize) * 1.1);     
  border-radius: 50%;
  background: var(--sliderThumb); 
  cursor: pointer;
  -webkit-appearance: none;
  margin-top: calc(-1.1 * var(--elmHalf));
}

input[type=range]:focus::-webkit-slider-runnable-track {
  background: var(--sliderBackground); 
}

.range-value{
  position: absolute;
  top: -60%;
}

.range-value span{
  width: var(--elmDbl);
  height: var(--elmSize);
  line-height: var(--elmSize);
  text-align: center;
  background: var(--sliderThumb); 
  color: var(--sliderText); 
  font-size: calc(var(--elmSize) * 0.9);
  font-weight: bold;
  display: block;
  position: absolute;
  left: 50%;
  transform: translate(-50%, 0);
  border-radius: var(--elmQuart);
  top: var(--elmHalf);            
}

.range-max,.range-min {
  display: inline-block;
  padding: 0 var(--elmQuart);
}

/* checkbox toggle switch slider */            
.switch {
  position: relative;
  display: inline-block;
  width: calc(var(--elmSize) * 1.9);
  height: calc(var(--elmSize) * 1.3);
  /*top: var(--elmHalf);*/
}

.switch input {
  opacity: 0;
  width: 0;
  height: 0;
}

.slider {
  position: absolute;
  border-radius: var(--elmSize);
  cursor: pointer;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: var(--toggleInactive);
  width: calc(var(--elmSize) * 2.3);                      
  transition: .4s;
  width: 100%
}

.slider:before {
  position: absolute;
  border-radius: 50%;
  content: "";
  height: var(--elmSize);
  width: var(--elmSize);
  left: var(--elmQuart);
  top: calc(var(--elmQuart)*.5);
  background: var(--sliderBackground);
  transition: .4s;
}

input:checked + .slider {
  background-color: var(--toggleActive);
}
input:focus + .slider {
  outline: auto;
}

input:checked + .slider:before {
  transform: translateX(calc(var(--elmSize) * 0.5));
}

select {
  outline: 0;
  min-width: calc(var(--elmSize) * 11);
  border-radius: var(--elmQuart);
  height: var(--inputHeight);
  margin-top: 2px;
  width: 99%
}
select:focus{
  outline: auto;
}
.selectField {
  height: var(--inputHeight);
}
</style>)=====";

PROGMEM const char CONFIGASSIST_HTML_SCRIPT[] = R"=====(
<script>
const $ = document.querySelector.bind(document);
const $$ = document.querySelectorAll.bind(document);
document.addEventListener('DOMContentLoaded', function (event) {
  let scanTimer = null;
  let setTimeTimer = null;
  let refreshInterval = 15000;
  const toggleAcord = $("#toggleAcord");
  
  toggleAcord.addEventListener('click', function (event) {
    if(event.target.classList.contains("open")){
        event.target.classList.remove("open");   
        event.target.classList.add("closed"); 
        $$('.card-header').forEach(el => {
            el.parentElement.classList.remove("open");
            el.parentElement.classList.add("closed");
        });  
    }else{
        event.target.classList.remove("closed");   
        event.target.classList.add("open");   
        $$('.card-header').forEach(el => {
            el.parentElement.classList.remove("closed");
            el.parentElement.classList.add("open");
        }); 
    }     
  });
  
  function updateRange(range) {
    const rangeVal = $('#'+range.id).parentElement.children.rangeVal;
    const rangeFontSize = parseInt(window.getComputedStyle($('input[type=range]')).fontSize); 
    let position = (range.clientWidth - rangeFontSize) * (range.value - range.min) / (range.max - range.min);
    position += range.offsetLeft + (rangeFontSize / 2);
    rangeVal.innerHTML = '<span>'+range.value+'</span>';
    rangeVal.style.left = 'calc('+position+'px)';
  }

  function closeAllCards(target, toggle=false){
    $$('.card-header').forEach(el => {
      if(el != target)        
        el.parentElement.classList.add("closed");
      else
        if(!toggle) el.parentElement.classList.remove("closed");
          //el.parentElement.classList.toggle("closed");
        //else 
//          el.parentElement.classList.remove("closed");
    });  
  }
  
  // Accordion
  $$('.card-header').forEach(el => {
    if(el.parentElement.classList.contains("accordion")){
      if(el.parentElement.classList.contains("toggle")){
        el.addEventListener("click", function (event) {
          closeAllCards(event.target.parentElement, true)
        });    
      }else{
        el.addEventListener("click", function (event) {
          closeAllCards(event.target.parentElement)
        });
      }
    }
  });

  //Update all ranges
  $$('input[type=range]').forEach(el => {updateRange(el);});

  // input events
  document.addEventListener("input", function (event) {
    if (event.target.type === 'range') updateRange(event.target);
  });

  // recalc range markers positions 
  window.addEventListener('resize', function (event) {
    $$('input[type=range]').forEach(el => {updateRange(el);});
  });
  
  // Add button handlers
  $$('button[type=button]').forEach(el => {    
    el.addEventListener('click', function (event) {    
       if(event.returnValue) updateKey(event.target.name, 1);
    });
  });

  // Add password view handlers
  $$('input[type=password]').forEach(el => {    
    el.addEventListener('keyup', function (event) {    
       var no = getKeyNo(event.target.name);
       const passV = $('#_PASS_VIEW' + no + "_GRP");
       if(passV)
          passV.style.display = "block";
    });
    el.addEventListener('focus', function (event) {    
       this.select();
    });
  });

  // Add dblClick handlers
  $$('input').forEach(el => {    
    el.addEventListener('dblclick', function (event) {    
       updateKey(event.target.name, event.target.value);
    });
  });
 
   // onChange events
  document.addEventListener("change", function (event) {
    const e = event.target;
    var value = e.value.trim();
    const et = event.target.type;
    
    if( e.name.includes("_PASS_VIEW") ){
      var no = getKeyNo(e.name);
      const p = e.parentElement.parentElement.parentElement.children[1].children[0]
      if(e.checked) p.type = "text";
      else p.type = "password";
      return;
    }
    
    if( e.name.includes("{CA_PASSWD_KEY}") ){
      const no = getKeyNo(e.name);
      const passV = $('#_PASS_VIEW' + no + "_GRP");
      if(passV)
        passV.style.display = "block";
    }

    if (e.nodeName == 'INPUT' || e.nodeName == 'SELECT') {  
      if (e.type === 'checkbox') updateKey(e.id, e.checked ? 1 : 0);
      else updateKey(e.id, value);
    }else if(e.nodeName == 'TEXTAREA'){
      const fileKey = e.name + "_F";
      value += "&"+fileKey+"=" + $('#'+fileKey).value;
      updateKey(e.id, value);
    }
  });

  function sleep(delay) {
    var start = new Date().getTime();
    while (new Date().getTime() < start + delay);
  }
  
  // Save config before unload
  window.addEventListener('beforeunload', function (event) {
    updateKey("_SAVE", 1);    
    console.log("Unload.. Saved ini");
    sleep(500);
  });

  function getKeyNo(key){
    n = key[key.length - 1];
    no = "";
    if(parseInt(n)) no += n;
    return no;
  }

  async function updateKey(key, value) {      
    if(value == null ) return;      
    const baseHost = document.location.origin;
    var url = baseHost + "/cfg?" + key + "=" + value
    
    if (key=="_RBT"){
      let nowUTC = Math.floor(new Date().getTime() / 1000);
      document.location = url+"&_TS="+ nowUTC;
    }else if ( key=="_DWN" || key=="_UPG" || key=="_FWC" || key.includes('_PASS_VIEW') ){
      return;
    }
    //console.log("Request ", url);
    const response = await fetch(encodeURI(url));
    if (!response.ok){
      const html = await response.text();
      if(html!="") $('#msg').innerHTML = html;
    }else if(key=="_RST"){
      const html = await response.text();
      if(html.startsWith("OK:"))      
        document.location = baseHost + "/cfg"
      else
        console.log(html)
    }
  }
  /*{SUB_SCRIPT}*/   
})
</script>)=====";

#ifdef CA_USE_TIMESYNC 
PROGMEM const char CONFIGASSIST_HTML_SCRIPT_TIME_SYNC[] = R"=====(
  async function sendTime() {
    let now = new Date();
    let nowUTC = Math.floor(now.getTime() / 1000);
    let offs = now.getTimezoneOffset()/60;
    const baseHost = document.location.origin;
    const url = baseHost + "/cfg?clockUTC" + "=" + nowUTC + "&clockOffs=" + offs
    const response = await fetch(encodeURI(url));
    if (!response.ok){
      const html = await response.text();
      if(html!="" && $('#msg')) $('#msg').innerHTML = html;
    }
  }
  
  setTimeTimer = setTimeout(sendTime, 500);  
)=====";
#endif

#ifdef CA_USE_TESTWIFI 
PROGMEM const char CONFIGASSIST_HTML_SCRIPT_TEST_ST_CONNECTION[] = R"=====(
  async function testWifi(no="") {
    const baseHost = document.location.origin;
    const url = encodeURI( baseHost + "/cfg?_TEST_WIFI="+no )      
    document.body.style.cursor  = 'wait';
    const ed = "st_ssid" + no;
    const lbl = ed + "-lbl"
    const res = ed + "_res"
    const res_link = ed + "_res_link"
    if($("#_TEST_WIFI"+no).innerHTML == "Testing..") return;
    $("#_TEST_WIFI"+no).innerHTML = "Testing.."
    if($("#"+res)) $("#"+res).innerHTML = ""
    if($("#"+res_link)) $("#"+res_link).style.display = "none";
    try {
      const response = await fetch(url);
      if (response.ok){
        var j = await response.json(); 
        if(!$("#"+res)){
            $("#"+lbl).innerHTML += "<br><span id=\"" + res + "\"></span>&nbsp;"
            $("#"+lbl).innerHTML += "<a target=\"_blank\" style=\"display: none\" id=\"" + res_link + "\" href=\"\">" + "</a>"
        }
      
        if(j.status=="Success"){
          $("#"+res).innerHTML = "<font color='Green'><b>" + j.status + "</b></font>";
          $("#"+res).innerHTML +="&nbsp;Rssi: " + j.rssi
          $("#"+res).innerHTML += "&nbsp;IP: "
          $("#"+res_link).innerHTML = j.ip
          $("#"+res_link).href = "http://" + j.ip;
          $("#"+res_link).style.display = "";
        }else{
          $("#"+res).innerHTML = "<font color='red'><b>" + j.status + "</b></font>";
          $("#"+res).innerHTML += "&nbsp;Code: " + j.code;
          $("#"+res_link).style.display = "none;";
        }        
      }else{
        alert("Fail: " + response.text());        
      }
    } catch(e) {
      alert(e + "\n" + url)
    }
    $("#_TEST_WIFI"+no).innerHTML = "Test connection"
    document.body.style.cursor  = 'default';
    //if($('#msg')) $('#msg').innerHTML = JSON.stringify(j);
  }
)=====";
#endif

#ifdef CA_USE_WIFISCAN 
PROGMEM const char CONFIGASSIST_HTML_SCRIPT_WIFI_SCAN[] = R"=====(
async function getWifiScan() {      
    const baseHost = document.location.origin;
    const url = baseHost + "/scan"
    try{
      const response = await fetch(encodeURI(url));
      var json = await response.json();       
      if (!response.ok){        
        console.log('Error:', json)
      }else{
        if(json.length == 1 && Object.keys(json[0]).length < 2 ) return;
        json = json.sort((a, b) => {
          if (a.rssi > b.rssi) {
            return -1;
          }
        });
        var options = "";
        for(n in json){
          options  += '<option value="' + json[n].ssid + '"></option>\n'
        }
        
        eds = document.querySelectorAll("input[id*='st_ssid']")        
        for (i = 0; i < eds.length; ++i) {
          ed = eds[i]
          id = ed.id.replace("st_ssid","")
          const td = ed.parentElement
        
          if(!ed.getAttribute('list')){
            list = document.createElement('datalist')
            list.id = 'st_wifi_list'+id
            td.appendChild(list)
            $('#st_wifi_list'+id).innerHTML =  options
            ed.setAttribute('list','st_wifi_list'+id)
          }else{
            $('#st_wifi_list'+id).innerHTML =  options
          }
        }
      }
    } catch(e) {
      console.log(e)
    } 
    clearTimeout(scanTimer);
    scanTimer = setTimeout(getWifiScan, refreshInterval);
  }
  
  scanTimer = setTimeout(getWifiScan, 2000); 
)=====";
#endif

// Template for password view check group
PROGMEM const char CONFIGASSIST_HTML_CHECK_VIEW_PASS[] = 
R"=====(<span id="{key}_GRP" style="display: none;">
<input type="checkbox" style="height:auto; min-width:auto; width:auto;" id="{key}" name="{key}" {chk}/>
<label for="{key}">{label}</label></span>
)=====";

// Template for one input text box
PROGMEM const char CONFIGASSIST_HTML_TEXT_BOX[] = 
R"=====(<input id="{key}" name="{key}" length="64" value="{val}">)=====";

// Template for one input text area
PROGMEM const char CONFIGASSIST_HTML_TEXT_AREA[] = 
R"=====(<textarea id="{key}" name="{key}" rows="auto" cols="auto">{val}</textarea>)=====";

// Template for one input text area file name
PROGMEM const char CONFIGASSIST_HTML_TEXT_AREA_FNAME[] = 
R"=====(<input type="hidden" id="{key}" name="{key}" value="{val}">)=====";

// Template for one input check box 
PROGMEM const char CONFIGASSIST_HTML_CHECK_BOX[] = R"=====(
            <div class="card-val-ctrl">              
              <div class="switch">
                  <input id="{key}" name="{key}" type="checkbox"{chk}>
                  <label class="slider" for="{key}"></label>
              </div>
            </div>)=====";

// Template for one input select box
PROGMEM const char CONFIGASSIST_HTML_SELECT_BOX[] = R"=====(
            <select id="{key}" name="{key}" style="width:100%">
              {opt}
            </select>)=====";

// Template for one input select option
PROGMEM const char CONFIGASSIST_HTML_SELECT_OPTION[] = 
R"=====(            <option value="{optVal}"{sel}>{optName}</option>
)=====";

// Template for one input select datalist option
PROGMEM const char CONFIGASSIST_HTML_SELECT_DATALIST_OPTION[] = 
R"=====(            <option value="{optVal}"></option>
)=====";

// Template for one input select datalist
PROGMEM const char CONFIGASSIST_HTML_DATA_LIST[] = 
R"=====(<input type="text" id="{key}" name="{key}" list="{key}_list" value="{val}"/>
          <datalist id="{key}_list">
              {opt}
          </datalist>)=====";

// Template for one input select range <label for="{key}">{key}</label>
PROGMEM const char CONFIGASSIST_HTML_INPUT_RANGE[] = R"=====(
            <div class="card-val-ctrl">              
              <div class="range-min">{min}</div>
              <input title="{lbl}" type="range" id="{key}" name="{key}" min="{min}" max="{max}" value="{val}">
              <div class="range-value" name="rangeVal"></div>
              <div class="range-max">{max}</div>            
            </div>)=====";

// Template html body
PROGMEM const char CONFIGASSIST_HTML_BODY[] = R"=====(
</head> 
<body>
<div class="container">
  <div class="row">
    <div class="column">
      <div class="card">
        <h2><font style="color: darkblue;">{host_name}</font> config</h2>
        <div title="Colapse / Expand" id="toggleAcord" class="accordBtt open"></div>
      </div>)=====";

// Template for one line
PROGMEM const char CONFIGASSIST_HTML_LINE[] = R"=====(
          <tr>
            <td class="card-key">{key}</td>
            <td class="card-val">{elm}</td>
            <td class="card-lbl">{lbl}</td>
          </tr>)=====";

// Seperator line
PROGMEM const char CONFIGASSIST_HTML_SEP[] = R"=====(
      <div class="card">
        <div class="card-header" onClick="this.parentElement.classList.toggle('closed')">
          <h3>{val}</h3>
        </div>
        <div class="card-body">
        <table>)=====";

// Close the seperator
PROGMEM const char CONFIGASSIST_HTML_SEP_CLOSE[] = R"=====(
        </table>
        </div> <!-- card-body -->
      </div><!-- card -->)=====";

// Template for page end
PROGMEM const char CONFIGASSIST_HTML_END[] = R"=====(
      </table>
     </div> <!-- card-body -->
     </div> <!-- card -->
     <div class="card">
        <button type="button" title="Go to home" onClick="window.location.href = '/'" name="_SAVE">Home</button>
        <button type="button" title="Reboot esp device" onClick="if(!confirm('Reboot esp?')) return false;" name="_RBT">Reboot</button>
        <button type="button" title="Backup configuration" onClick="window.location.href = '/cfg?_DWN=1'" name="_DWN">Backup</button>
        <button type="button" title="Restore configuration" onClick="window.location.href = '/upl'" name="_UPL">Restore</button>
        <button type="button" title="Reset values to defaults" onClick="if(!confirm('Reset to default values?')) return false;" name="_RST">Defaults</button>
        <!--extraButtons-->
     </div> <!-- card -->
    </div> <!-- column -->
  </br>
  <center><small id="msg" style="color: lightgray;">ConfigAssist Ver: {appVer}</small></center>
  </br>
  </div> <!-- row -->
</div> <!-- container -->
</body>
</html>
)=====";

// OTA Upload button
PROGMEM const char HTML_UPGRADE_BUTTON[] = R"~(
  <button type="button" title="Upload firmware to upgrade" onClick="window.location.href = '/ota'" name="_UPG">Upgrade</button>
)~";

// Firmware check from url button
PROGMEM const char HTML_FIRMWCHECK_BUTTON[] = R"~(
  <button type="button" title="Online firmware check" onClick="window.location.href = '/fwc'" name="_FWC">Firmware</button>
)~";