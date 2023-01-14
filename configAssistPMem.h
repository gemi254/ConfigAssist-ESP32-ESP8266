// Minimal application config dictionary
const char* appDefConfigDict_json PROGMEM = R"~(
[{
      "name": "st_ssid",
     "label": "Enter the name WLAN to connect",
   "default": ""
  },
  {
      "name": "st_pass",
     "label": "Password for station WLAN",
   "default": ""
  },
  {
      "name": "host_name",
     "label": "Enter a name for your host",
   "default": ""
  }]
)~";

// Template for header, begin of the config form
PROGMEM const char HTML_PAGE_START[] = R"=====(
<!DOCTYPE HTML>
<html lang='de'>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Configuration for {appName}</title>
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
.pair-key{
    text-align: right;
    font-weight: 800;
}
.pair-val{
    text-align: left;
}
.pair-lbl{
    text-align: left;
    font-style: italic;}


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
    margin-bottom: 0.625em;
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
.dcf-w-100\% {
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
  <div class="dcf-modal-content dcf-wrapper dcf-pb-8 dcf-w-100">
    <div class="cf-w-100">
      <form method='post'>
      <table>
        <caption>
          <h3>Config for {appName}</h3>
        </caption>
        <tbody>
)=====";

//Template for one input field
PROGMEM const char HTML_PAGE_INPUT_LINE[] = R"=====(
    <tr>
      <td scope="row" class="pair-key">{key}</td>
      <td class="pair-val">
        <input id="{key}" name="{key}" length="64" value="{val}">
       </td>
      <td class="pair-lbl">{lbl}</td>
    </tr>    
)=====";

//Template for save button and end of the form with save
PROGMEM const char HTML_PAGE_END[] = R"=====(
      </tbody>
          <tfoot>
            <tr>
              <td style="text-align: center;" colspan="5">
                  <button type='submit' name='SAVE'>Save</button>
                  <button type='submit' onClick='if(!confirm("Reboot esp?")) return false;' name='RBT'>Reboot ESP</button>
                  <button type='submit' onClick='if(!confirm("Reset values?")) return false;' title='Reset values to default' name='RST'>Reset</button>
                  <button type='submit' onClick='if(!confirm("Discard changes?")) return false;' name='CANCEL'>Cancel</button>
              </td>
            </tr>
          </tfoot>
        </table>
        </form>
      </div>
  </div>
</center>
</body>
</html>
)=====";

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
