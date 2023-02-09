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

// Template for header, begin of the config form
PROGMEM const char HTML_PAGE_START[] = R"=====(
<!DOCTYPE HTML>
<html lang='de'>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Configuration for {host_name}</title>
<style>
:root {
 --bg-table-stripe: #f6f6f5;
 --b-table: #999a9b47;
 --caption: #242423;
}
body {
  font-family: "Open Sans", sans-serif;
  line-height: 1.25;
}
table {
  border-bottom: 1px solid var(--b-table);
  border-top: 1px solid var(--b-table);  
  border-collapse: collapse;
  margin: 0;
  padding: 0;
  table-layout: fixed;
}
table caption {  
  margin: .4em 0 .35em;
  color: var(--caption);
}
table tfoot{
  padding: 5px;  
}
table th, table td {
  padding: .525em;
  border: 1px solid var(--b-table);
}
tbody tr:nth-of-type(2n+1) {
  background-color: var(--bg-table-stripe)
}
.pair-sep{
    text-align: center;
    font-weight: 800;
    font-size: 16px;
    border-bottom: 2px solid lightgray;
    background-color: beige;
}
.pair-key{
    text-align: right;
    font-weight: 800;
}
.pair-val{
    text-align: left;
}
.pair-lbl{
    text-align: left;
    font-style: italic;
    font-size: .8em;
}

 @media screen and (max-width: 400px) {
  table {
    border: 0;
  }

  table caption {
    font-size: 1.3em;
  }

  table thead {
    border: none;
    clip: rect(0 0 0 0);
    height: 1px;
    margin: -1px;
    overflow: hidden;
    padding: 0;
    position: absolute;
    width: 1px;
  }

  table tr {
    border-bottom: 3px solid #ddd;
    display: block;
    margin-bottom: 0.125em;
  }

  table td {
    border-bottom: 1px solid #ddd;
    display: block;
    font-size: 0.8em;
    text-align: right;
  }

  table td::before {
    content: attr(data-label);
    float: left;
    font-weight: bold;
    text-transform: uppercase;
  }

  table td:last-child {
    border-bottom: 0;
  }
  .pair-key{
    text-align: left;    
  }
}
input[type="range"] {
  width:100%;
}
.w-100 {
  width: 100%!important;
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
</style>
</head>
<body>
<center>
    <div class="w-100">
      <form method='post'>
      <table>
        <caption>
          <h3>Config for {host_name}</h3>
        </caption>
        <tbody>)=====";

//Template for seperator line
PROGMEM const char HTML_PAGE_SEPERATOR_LINE[] = R"=====(
        <tr>
            <td colspan="5" class="pair-sep">{val}</td>
        </tr>    
)=====";

//Template for one configuration pair
PROGMEM const char HTML_PAGE_INPUT_LINE[] = R"=====(
        <tr>
          <td scope="row" class="pair-key">{key}</td>
          <td class="pair-val">{elm}</td>
          <td class="pair-lbl">{lbl}</td>
        </tr>)=====";

//Template for one input text box
PROGMEM const char HTML_PAGE_TEXT_BOX[] = 
R"=====(<input id="{key}" name="{key}" length="64" value="{val}">)=====";

//Template for one input text area
PROGMEM const char HTML_PAGE_TEXT_AREA[] = 
R"=====(<textarea id="{key}" name="{key}" rows="auto" cols="auto">{val}</textarea>)=====";

//Template for one input text area file name
PROGMEM const char HTML_PAGE_TEXT_AREA_FNAME[] = 
R"=====(<input type="hidden" name="{key}" value="{val}">)=====";

//Template for one input check box
PROGMEM const char HTML_PAGE_CHECK_BOX[] = 
R"=====(<input type='checkbox' name='{key}'{chk}>)=====";

//Template for one input select box
PROGMEM const char HTML_PAGE_SELECT_BOX[] = R"=====(
            <select name='{key}' style='width:100%'>
              {opt}
            </select>
)=====";

//Template for one input select option
PROGMEM const char HTML_PAGE_SELECT_OPTION[] = 
R"=====(            <option value='{optVal}'{sel}>{optVal}</option>
)=====";

//Template for one input select datalist option
PROGMEM const char HTML_PAGE_SELECT_DATALIST_OPTION[] = 
R"=====(            <option value='{optVal}'></option>
)=====";

//Template for one input select datalist
PROGMEM const char HTML_PAGE_DATA_LIST[] = 
R"=====(<input type="text" name="{key}" list="{key}_list" value="{val}"/>
          <datalist id='{key}_list'>
              {opt}
          </datalist>
)=====";

//Template for one input select range
PROGMEM const char HTML_PAGE_INPUT_RANGE[] = R"=====(
            <input type='range' min='{min}' max='{max}' step='{step}' value='{val}' name='{key}'>)=====";

//Template for save button and end of the form with save
PROGMEM const char HTML_PAGE_END[] = R"=====(
      </tbody>
          <tfoot>
            <tr>
              <td style="text-align: center;" colspan="5">
                  <button type='submit' title='Save configuration file to storage' name='SAVE'>Save</button>
                  <button type='submit' title='Reboot esp device' onClick='if(!confirm("Reboot esp?")) return false;' name='RBT'>Reboot</button>
                  <button type='submit' title='Discard changes and return to home page' onClick='if(!confirm("Discard changes?")) return false;' name='CANCEL'>Home</button>
                  <button type='submit' title='Reset values to defaults' onClick='if(!confirm("Reset values?")) return false;' name='RST'>Defaults</button>
              </td>
            </tr>
          </tfoot>
        </table>
        </form>
      </div>
</center>
</body>
</html>
)=====";

//Template for message page
PROGMEM const char HTML_PAGE_MESSAGE[] = R"=====(
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
