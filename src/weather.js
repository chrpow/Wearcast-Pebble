var myAPIKey = '025208b931ba89abab320d0e2ed73b69';

var configuration = JSON.parse(decodeURIComponent(e.response));

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  // Construct URL
  var url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
      pos.coords.latitude + "&lon=" + pos.coords.longitude + '&appid=' + myAPIKey;

  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      // Temperature in Kelvin requires adjustment
      var temperature = Math.round((json.main.temp - 273.15) * 1.8 + 32);
      console.log("Temperature is " + temperature);

      // Conditions
      var conditions = json.weather[0].main;      
      console.log("Conditions are " + conditions);
      
      // compute values to return
      var head = headCalculator(temperature, conditions);
      var chest = chestCalculator(temperature, conditions);
      var legs = legsCalculator(temperature, conditions);
      var umbrella = umbrellaCalculator(conditions);
      
      console.log("Clothes are" + head + ""+ chest + "" + legs + "" + umbrella + "");
      
      // Assemble dictionary using our keys
      var dictionary = {
        "KEY_TEMPERATURE": temperature,
        "KEY_CONDITIONS": conditions.toUpperCase()
        "KEY_HEAD" : head;
        "KEY_CHEST" : chest;
        "KEY_LEGS" : legs;
        "KEY_UMBRELLA" : umbrella;
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Weather info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending weather info to Pebble!");
        }
      );
    }      
  );
}

function chestCalculator(temp, cond) {
  var weather = cond.toLowerCase;
  if (weather == 'snow' || temp <= 42)
    return 1; // coat
  else if (weather == 'rain')
    return 2; // rain jacket
  else if (temp <= 50)
    return 3; // sweater
  else if (temp <= 60)
    return 4; // long sleeve
  else
    return 5; // short sleeve
}

function legsCalculator(temp, cond) {
  var weather = cond.toLowerCase;
  if (temp >= 60 && weather != 'snow' && weather != 'rain')
    return 1; // pants shoes
  else if (weather == 'snow' || weather == rain)
    return 2; // pants boots
  else
    return 3; // shorts shoes
}

function headCalculator(temp, cond) {
  var weather = cond.toLowerCase;
  if (temp <= 42 || weather == 'snow' || weather == 'rain')
    return 1; // hat
  else
    return 2; //none
}

function umbrellaCalculator(cond) {
  var weather = cond.toLowerCase;
  if (weather == umbrella)
    return 1; // umbrella
  else
    return 2; // none
}

function locationError(err) {
  console.log("Error requesting location!");
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");

    // Get the initial weather
    getWeather();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    getWeather();
  }                     
);
