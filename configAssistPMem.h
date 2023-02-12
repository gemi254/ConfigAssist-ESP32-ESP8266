// Minimal application config dictionary
const char* appDefConfigDict_json PROGMEM = R"~(
[{
      "name": "st_ssid",
     "label": "Enter the name WLAN to connect",
   "default": ""
  },{
      "name": "st_pass",
     "label": "Password for station WLAN",
   "default": ""
  },{
      "name": "host_name",
     "label": "Enter a name for your host",
   "default": "SetupAssist_{mac}"
  }])~";

//Template for one input text box
PROGMEM const char CONFIGASSIST_HTML_TEXT_BOX[] = 
R"=====(<input id="{key}" name="{key}" length="64" value="{val}">)=====";

//Template for one input text area
PROGMEM const char CONFIGASSIST_HTML_TEXT_AREA[] = 
R"=====(<textarea id="{key}" name="{key}" rows="auto" cols="auto">{val}</textarea>)=====";

//Template for one input text area file name
PROGMEM const char CONFIGASSIST_HTML_TEXT_AREA_FNAME[] = 
R"=====(<input type="hidden" name="{key}" value="{val}">)=====";

//Template for one input check box
PROGMEM const char CONFIGASSIST_HTML_CHECK_BOX[] = 
R"=====(<input type='checkbox' name='{key}'{chk}>)=====";

//Template for one input select box
PROGMEM const char CONFIGASSIST_HTML_SELECT_BOX[] = R"=====(
            <select name='{key}' style='width:100%'>
              {opt}
            </select>
)=====";

//Template for one input select option
PROGMEM const char CONFIGASSIST_HTML_SELECT_OPTION[] = 
R"=====(            <option value='{optVal}'{sel}>{optVal}</option>
)=====";

//Template for one input select datalist option
PROGMEM const char CONFIGASSIST_HTML_SELECT_DATALIST_OPTION[] = 
R"=====(            <option value='{optVal}'></option>
)=====";

//Template for one input select datalist
PROGMEM const char CONFIGASSIST_HTML_DATA_LIST[] = 
R"=====(<input type="text" name="{key}" list="{key}_list" value="{val}"/>
          <datalist id='{key}_list'>
              {opt}
          </datalist>
)=====";

//Template for one input select range
PROGMEM const char CONFIGASSIST_HTML_INPUT_RANGE[] = R"=====(
            <input type='range' min='{min}' max='{max}' step='{step}' value='{val}' name='{key}'>)=====";

//Template for message page
PROGMEM const char CONFIGASSIST_HTML_MESSAGE[] = R"=====(
<!DOCTYPE html>
<html lang="en" class="">
    <head>
      <meta charset='utf-8'>
        <meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no"/>
        <title>{title}</title>
        <script>
          setTimeout(function() {
                      location.href = '{url}';
                  }, {refresh});
        </script>                  
    </head>
    <body>
    <div style='text-align:center;'><h3>{msg}</h3></div>
    </div></body></html>
)=====";

// Template for header, begin of the config form
PROGMEM const char CONFIGASSIST_HTML_START[] = 
R"=====(<!DOCTYPE HTML>
<html lang='de'>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Configuration for {host_name}</title>)=====";

// Template for header, begin of the config form
PROGMEM const char CONFIGASSIST_HTML_CSS[] = 
R"=====(<style>
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
/* Clear floats after the columns */
.row:after {
  content: "";
  display: table;
  clear: both;
}
.column .card.closed{
    height: 26px;
    overflow: hidden;
}
.column .card.closed h3{
  color: darkslategray;
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
.card h3{
  margin:2px;
  color: darkgray;
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
.card-lbl {
  text-align: left;
  font-style: italic;
  font-size: .8em;
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

// Template html body
PROGMEM const char CONFIGASSIST_HTML_BODY[] = R"=====(
</head> 
<body>
<div class="container">
  <div class="row">
    <div class="column">
    <form method='post'>
      <div class="card">
        <h2><font style="color: darkblue;">{host_name}</font> config</h2>
      </div>)=====";

// Template for one line
PROGMEM const char CONFIGASSIST_HTML_LINE[] = R"=====(
          <tr>
            <td class="card-key">{key}</td>
            <td class="card-val">{elm}</td>
            <td class="card-lbl">{lbl}</td>
          </tr>)=====";
//Seperator line
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
      <table>
          <tr>
            <td style="text-align: center;" colspan="5">
              <button type='submit' title='Save configuration file to storage' name='SAVE'>Save</button>
              <button type='submit' title='Reboot esp device' onClick='if(!confirm("Reboot esp?")) return false;' name='RBT'>Reboot</button>
              <button type='submit' title='Discard changes and return to home page' onClick='if(!confirm("Discard changes?")) return false;' name='CANCEL'>Home</button>
              <button type='submit' title='Reset values to defaults' onClick='if(!confirm("Reset values?")) return false;' name='RST'>Defaults</button>
            </td>
        </tr>
      </table>
     </div> <!-- card -->
     </form>
    </div> <!-- column -->
  </br>
  <center><small>ConfigAssist Ver: {appVer}</small></center>
  </br>
  </div> <!-- row -->
</div> <!-- container -->
</body>
</html>
)=====";
