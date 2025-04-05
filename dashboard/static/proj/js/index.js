
let map;
let markers = [];

// Initialize Google Map
function initMap() {
    map = new google.maps.Map(document.getElementById("map"), {
        center: { lat: 1.3521, lng: 103.8198 }, // Default center (Singapore)
        zoom: 12,
    });

    fetchAndUpdateBins(); // Load bins initially
    setInterval(fetchAndUpdateBins, 10000); // Refresh every 60 seconds
}

window.initMap = initMap;

// Fetch bin data and update map
function fetchAndUpdateBins() {
    console.log("Fetching bins...");
    fetch('/get_bins')
        .then(response => response.json())
        .then(data => {

            markers.forEach(marker => marker.setMap(null)); // Remove old markers
            markers = [];

            data.bins.forEach(bin => {
                let marker = new google.maps.Marker({
                    position: { lat: bin.lat, lng: bin.lon },
                    map: map,
                    title: `Bin ${bin.id} - ${bin.location}`,
                });
                console.log(bin);

                let infoWindow = new google.maps.InfoWindow({
                    content: `
                        <div>
                            <h6><strong>Bin Name: ${bin.device_name}</strong></h6>
                            <p><strong>Active:</strong> ${bin.active}</p>
                            <p><strong>Fill Level:</strong> ${bin.fill_level}%</p>
                            <p><strong>Temperature:</strong> ${bin.temperature}°C</p>
                            <p><strong>Humidity:</strong> ${bin.humidity}%</p>
                            <p><strong>Smoke Concentration:</strong> ${bin.smoke_concentration} PPM</p>
                            <p><strong>Last Updated:</strong> ${bin.received_at}</p>
                            <p><strong>Status:</strong> 
                                <span class="badge bg-${bin.active === 'Yes' ? 'success' : 'secondary'} text-white">${bin.active === 'Yes' ? 'ON' : 'OFF'}</span>
                            </p>
                            ${bin.anomaly !== "No" ? `<p class="text-danger"><strong>⚠️ Anomaly detected: ${bin.anomaly}</strong></p>` : ''}
                        </div>
                    `,
                });



                marker.addListener("click", () => {
                    infoWindow.open(map, marker);
                });

                markers.push(marker);
            });
            updateCharts(data);
        })

        .catch(error => console.error("Error fetching bin data:", error));
}



function updateCharts(data) {
    console.log("Updating charts...");

    Chart.defaults.global.defaultFontFamily = 'Nunito', '-apple-system,system-ui,BlinkMacSystemFont,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif';
    Chart.defaults.global.defaultFontColor = '#858796';

    console.log(data)
    let inactive_bins = data.inactive_bins
    let full_bins = data.full_bins
    let active_bins = data.active_bins_graph

    // Pie Chart Example
    var ctx = document.getElementById("myPieChart");
    var myPieChart = new Chart(ctx, {
        type: 'doughnut',
        data: {
            labels: ["Active", "Inactive", "Full"],
            datasets: [{
                data: [active_bins, inactive_bins, full_bins],
                backgroundColor: ['#1cc88a', '#858181', '#ccb011'],
                hoverBackgroundColor: ['#1da373', '#404040', '#8f7900'],
                hoverBorderColor: "rgba(234, 236, 244, 1)",
            }],
        },
        options: {
            maintainAspectRatio: false,
            tooltips: {
                backgroundColor: "rgb(255,255,255)",
                bodyFontColor: "#858796",
                borderColor: '#dddfeb',
                borderWidth: 1,
                xPadding: 15,
                yPadding: 15,
                displayColors: false,
                caretPadding: 10,
            },
            legend: {
                display: false
            },
            cutoutPercentage: 80,
        },
    });



    // Set new default font family and font color to mimic Bootstrap's default styling
    Chart.defaults.global.defaultFontFamily = 'Nunito', '-apple-system,system-ui,BlinkMacSystemFont,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif';
    Chart.defaults.global.defaultFontColor = '#858796';

    function number_format(number, decimals, dec_point, thousands_sep) {
        // *     example: number_format(1234.56, 2, ',', ' ');
        // *     return: '1 234,56'
        number = (number + '').replace(',', '').replace(' ', '');
        var n = !isFinite(+number) ? 0 : +number,
            prec = !isFinite(+decimals) ? 0 : Math.abs(decimals),
            sep = (typeof thousands_sep === 'undefined') ? ',' : thousands_sep,
            dec = (typeof dec_point === 'undefined') ? '.' : dec_point,
            s = '',
            toFixedFix = function (n, prec) {
                var k = Math.pow(10, prec);
                return '' + Math.round(n * k) / k;
            };
        // Fix for IE parseFloat(0.55).toFixed(0) = 0;
        s = (prec ? toFixedFix(n, prec) : '' + Math.round(n)).split('.');
        if (s[0].length > 3) {
            s[0] = s[0].replace(/\B(?=(?:\d{3})+(?!\d))/g, sep);
        }
        if ((s[1] || '').length < prec) {
            s[1] = s[1] || '';
            s[1] += new Array(prec - s[1].length + 1).join('0');
        }
        return s.join(dec);
    }


    var full_bin_history = data.full_bin_history;
    full_bin_history.sort((a, b) => {
        let hourA = parseInt(a.hour);
        let hourB = parseInt(b.hour);

        // If hourA is 00-09, shift it to after 23 (next day)
        if (hourA < 10) hourA += 24;
        if (hourB < 10) hourB += 24;

        return hourA - hourB;
    });
    var labels = full_bin_history.map(item => item.hour);
    var dataValues = full_bin_history.map(item => item.full_bins);

    // Area Chart Example
    var ctx = document.getElementById("myAreaChart");
    var myLineChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: labels,
            datasets: [{
                label: "No. of Bins",
                lineTension: 0.3,
                backgroundColor: "rgba(78, 115, 223, 0.05)",
                borderColor: "rgba(78, 115, 223, 1)",
                pointRadius: 3,
                pointBackgroundColor: "rgba(78, 115, 223, 1)",
                pointBorderColor: "rgba(78, 115, 223, 1)",
                pointHoverRadius: 3,
                pointHoverBackgroundColor: "rgba(78, 115, 223, 1)",
                pointHoverBorderColor: "rgba(78, 115, 223, 1)",
                pointHitRadius: 10,
                pointBorderWidth: 2,
                data: dataValues,
            }],
        },
        options: {
            maintainAspectRatio: false,
            layout: {
                padding: {
                    left: 10,
                    right: 25,
                    top: 25,
                    bottom: 0
                }
            },
            scales: {
                xAxes: [{
                    time: {
                        unit: 'date'
                    },
                    gridLines: {
                        display: false,
                        drawBorder: false
                    },
                    ticks: {
                        maxTicksLimit: 7
                    }
                }],
                yAxes: [{
                    ticks: {
                        maxTicksLimit: 5,
                        padding: 10,
                    },
                    gridLines: {
                        color: "rgb(234, 236, 244)",
                        zeroLineColor: "rgb(234, 236, 244)",
                        drawBorder: false,
                        borderDash: [2],
                        zeroLineBorderDash: [2]
                    }
                }],
            },
            legend: {
                display: false
            },
            tooltips: {
                backgroundColor: "rgb(255,255,255)",
                bodyFontColor: "#858796",
                titleMarginBottom: 10,
                titleFontColor: '#6e707e',
                titleFontSize: 14,
                borderColor: '#dddfeb',
                borderWidth: 1,
                xPadding: 15,
                yPadding: 15,
                displayColors: false,
                intersect: false,
                mode: 'index',
                caretPadding: 10,
            }
        }
    });
}

