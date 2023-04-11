 document.addEventListener('DOMContentLoaded', function() {
    refreshNetwork();
});

function showMessage(msg){
    document.getElementById('dlgTitle').innerText = "Information"
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

function visiblity(tagId, isDisabled){
    const tag = document.getElementById(tagId);
    if(tag){
        tag.disabled = isDisabled;
    }
}

function connectNetwork(){
    ssid = document.getElementById('ssid').value;
    pass = document.getElementById('password').value;
    if(!ssid || pass.length < 8){
        alert('Your input is not valid');
        return;
    }
    showProgressRing();
    visiblity('btnConnect', true);
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
        visiblity('btnConnect', false);
    });
}

function refreshNetwork(){
    showProgressRing();
    visiblity('btnRefresh', true);
    fetchData('/networks.json', 15000)
    .then(response => response.json())
    .then(data => {
        clearTable();
        data.forEach((item, row) => {
            addWiFi(++row, item.ssid, item.rssi, item.bssid, item.authmode, item.hidden);
    });
})
.catch(() => {
    showMessage("Can't fetch networks")
    console.log("error to fetch networks");
})
.finally(() => {
    hideProgressRing();
    visiblity('btnRefresh', false);
});
}

function settings(){
    document.getElementById('dlgTitle').innerText = "Settings"
    document.getElementById('dlgMessage').innerText = '';
    var btnClearE = document.createElement('button');
    btnClearE.className = 'button button-primary';
    btnClearE.innerText = 'Clear EEPROM';
    btnClearE.id = 'btnClearEEPROM';
    btnClearE.addEventListener('click', clearEEPROM);
    document.getElementById('dlgMessageOption').innerHTML = '';
    document.getElementById('dlgMessageOption').appendChild(btnClearE);
    document.querySelector(".dialog-overlay").style.display = "flex";
}

function clearEEPROM(){
    visiblity('btnClearEEPROM', true);
    var btn = document.getElementById('btnClearEEPROM');
    fetchData('/settings/clearEEPROM', 10000)
    .then(data =>{
        if(data.status === 200){
            btn.innerText = "Successfully";
            btn.className = "button button-success";
        }
        else{
            btn.innerText = "Failed";
            btn.className = "button button-danger";
        }
    })
    .catch(() => {
        alert("Can't clear EEPROM")
        console.log("error to clear eeprom");
    })
    .finally(() => {
        setTimeout(function(){
            btn.className = 'button button-primary';
            btn.innerText = 'Clear EEPROM';
            visiblity('btnClearEEPROM', false);
        }, 3000);
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
        if(value !== "*HIDDEN*")
            document.getElementById('ssid').value = value;
        var passwordTag = document.getElementById('password');
        passwordTag.value = '';
        passwordTag.focus();
    }  
}

function addWiFi(row, ssid,rssi, bssid, encryption, isHide){
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
    p.textContent = isHide ? "*HIDDEN*" : ssid;
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
    return `${enc} ${enc.toLowerCase() === 'open' ? '\u{1F513}' : '\u{1F510}'}`;
}

const fetchData = (url, timeout = 5000) => {
    return Promise.race([
      fetch(url),
      new Promise((_, reject) =>
        setTimeout(() => reject(new Error('Timeout')), timeout)
      )
    ]);
  };