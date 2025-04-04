// Call the dataTables jQuery plugin
$(document).ready(function() {
    $('#dataTable').DataTable();
  });

  function initMap() {
    map = new google.maps.Map(document.getElementById("map"), {
        center: { lat: 1.3521, lng: 103.8198 }, // Default center (Singapore)
        zoom: 12,
    });

    fetchAndUpdateBins(); // Load bins initially
    setInterval(fetchAndUpdateBins, 60000); // Refresh every 60 seconds
}


  function openBinMapFromLink(link) {
    let lat = parseFloat(link.getAttribute("data-lat"));
    let lon = parseFloat(link.getAttribute("data-lon"));
    let bin = JSON.parse(link.getAttribute("data-bin")); // Convert string to JSON

    openBinMap(lat, lon, bin);
}


// Function to open the map modal for a specific bin
function openBinMap(lat, lon, bin) {
  // Initialize modal map
  let modalMap = new google.maps.Map(document.getElementById("modalMap"), {
      center: { lat: lat, lng: lon },
      zoom: 18,
  });

  // Create a marker for the selected bin
  let marker = new google.maps.Marker({
      position: { lat: lat, lng: lon },
      map: modalMap,
      title: `Bin ${bin.id} - ${bin.device_name}`,
  });

  // Attach an InfoWindow with bin details
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
                  <span class="badge bg-${bin.active === 'Yes' ? 'success' : 'warning'}">${bin.active.toUpperCase()}</span>
              </p>
              ${bin.anomaly !== "No" ? `<p class="text-danger"><strong>⚠️ Anomaly detected: ${bin.anomaly}</strong></p>` : ''}
          </div>
      `,
  });

  // Open info window on marker click
  marker.addListener("click", () => {
      infoWindow.open(modalMap, marker);
  });

  // Auto-open info window
  infoWindow.open(modalMap, marker);

  // Show Bootstrap modal
  $("#mapModal").modal("show");
}
