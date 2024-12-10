<?php
if ($_SERVER['REQUEST_METHOD'] == 'POST' && !empty(file_get_contents('php://input'))) {
    $files = array_diff(scandir('uploads/'), array('.', '..'));
    $count = count($files);

    $target_dir = "uploads/";
    if (!file_exists($target_dir)) {
        mkdir($target_dir, 0777, true);  // Utwórz folder, jeśli nie istnieje
    }
    $photoName = $_GET['name'];

    $target_file = $target_dir . $photoName;

    $data = file_get_contents('php://input');

    file_put_contents($target_file, $data);
    echo "Photo saved successfully!";
} else {
    echo "No file recieved.";
}
?>