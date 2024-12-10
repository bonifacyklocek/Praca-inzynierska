<!DOCTYPE html>
<html>
<head>
    <title>Podgląd zdjęć z ESP32-CAM</title>
    <style>
    * {
    box-sizing: border-box;
    }

    .column {
    float: left;
    width: 20%;
    padding: 2px;
    }

    /* Clearfix (clear floats) */
    .row::after {
    content: "";
    clear: both;
    display: table;
    }
</style>
</head>
<body>
    <h1>Podgląd zdjęć z ESP32-CAM</h1>
    <?php
    $files = array_diff(scandir('uploads/'), array('.', '..'));
    $fileCount = count($files);
    $rest = $fileCount % 5;
    sort($files);
    array_splice($files, $fileCount - $rest, $rest);
    
    foreach ($files as $file) {
        if (intval($file) % 5 == 0) {
            echo '<div class="row">';
        }

        $image_path = 'uploads/' . $file;
        if (file_exists($image_path)) {
            echo '<div class="column">
                    <img src="'.$image_path.'" alt="Ostatnie zdjęcie" style="width:100%;">
                </div>';
        } else {
            echo "Brak zdjęcia do wyświetlenia.";
        }

        if (intval($file) % 5 == 4) {
            echo '</div>';
        }
    }
    ?>
</body>
</html>