<?php


// echo sprintf("%.2f", 6.5344);

// exit;
$html = "const char PAGE_INDEX[] PROGMEM = R\"=====(\r\n".file_get_contents('index.html')."\r\n)=====\";";
// $css = file_get_contents('style.css');
// $js = file_get_contents('main.js');


// $full_html = str_replace(['/*CSS-HERE*/', '//JS-HERE'], [$css, $js], $html);

file_put_contents('index.h', $html);

