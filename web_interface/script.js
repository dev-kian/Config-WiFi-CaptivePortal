 document.addEventListener('DOMContentLoaded', function() {
    refreshNetwork();
});

function showMessage(msg){
    document.querySelector(".dialog-overlay").style.display = "flex";
    document.getElementById('dlgMessage').innerText = msg;
}

function hideMessage() {
    document.querySelector(".dialog-overlay").style.display = "none";
}

function showProgressRing(){
    document.getElementById('loader-wrapper').style.display = 'block';
}

function hideProgressRing(){
    document.getElementById('loader-wrapper').style.display = 'none';
}

function clearTable(){
    document.querySelector("tbody").innerHTML = '';
}

function connectNetwork(){
    ssid = document.getElementById('ssid').value;
    pass = document.getElementById('password').value;
    if(!ssid || pass.length < 8){
        alert('Your input is not valid');
        return;
    }
    showProgressRing();
    document.querySelector('#btnConnect').disabled = true;
    var json = `{"s":"${ssid}", "p":"${pass}"}`;
    fetchData(`/connect?p=${btoa(json)}`, 20000)
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            var info = `\nSSID: ${data.ssid}\nIP: ${data.ip}\nSubnet: ${data.subnet}\nGateway: ${data.gateway}\n Mac: ${data.mac}`
            showMessage(`Connection Successfully.${info}`);
        } else {
            showMessage('Your SSID or Password is not valid!');
        }
    })
    .catch(error => {
        showMessage('Request failed');
    })
    .finally(() => {
        hideProgressRing();
        document.querySelector('#btnConnect').disabled = false;
    });
}

function refreshNetwork(){
    showProgressRing();
    document.querySelector('#btnRefresh').disabled = true;
    fetchData('/networks.json', 15000)
    .then(response => response.json())
    .then(data => {
        clearTable();
        data.forEach((item, row) => {
        addWiFi(++row, item.ssid, item.rssi, item.bssid, item.authmode);
    });
})
.catch(() => {
    showMessage("Can't fetch networks")
    console.log("error to fetch networks");
})
.finally(() => {
    hideProgressRing();
    document.querySelector('#btnRefresh').disabled = false;
});
}

function onClickItemTable(x){
    x.classList.add('selected');
    siblings = Array.from(x.parentNode.children);
    siblings.forEach((sibling) => {
    if (sibling !== x) {
        sibling.classList.remove('selected');
    }
});
    var value = x.querySelector('td p').textContent;
    if(value)
    {
        document.getElementById('ssid').value = value;
        document.getElementById('password').value = '';
        document.getElementById('password').focus();
    }  
}

function addWiFi(row, ssid,rssi, bssid, encryption){
    var newRow = document.createElement('tr');
    newRow.onclick = function() {
        onClickItemTable(this);
    };
    var th = document.createElement('th');
    var td1 = document.createElement('td');
    var p = document.createElement('p');
    var td2 = document.createElement('td');
    var td3 = document.createElement('td');
    var td4 = document.createElement('td');
    th.textContent = row;
    newRow.appendChild(th);
    p.textContent = ssid;
    td1.appendChild(p);
    newRow.appendChild(td1);
    td2.textContent = getSignal(rssi);
    newRow.appendChild(td2);
    td3.textContent = bssid;
    newRow.appendChild(td3);
    td4.textContent = encIcon(encryption);
    newRow.appendChild(td4);
    document.getElementById('table-body').appendChild(newRow);
}
function getSignal(rssi) {
    var percentage;
    if (rssi <= -100) {
      percentage = 0;
    } else if (rssi >= -50) {
      percentage = 100;
    } else {
      percentage = 2 * (rssi + 100);
    }
    return `${percentage}%`;
  }

function encIcon(enc){
    return `${enc} ${enc.toLowerCase() === 'open' ? '\u{1F513}' : '\u{1F510}'}`
}

const fetchData = (url, timeout = 5000) => {
    return Promise.race([
      fetch(url),
      new Promise((_, reject) =>
        setTimeout(() => reject(new Error('Timeout')), timeout)
      )
    ]);
  };