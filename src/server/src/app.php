<?php

require_once 'vendor/db.php';
require_once 'vendor/routes.php';

$db = new DBManager("localhost", "loader", "root", "", "utf8mb4");
$route = new RouteManager();

$route->post('/api/auth', function() use ($db) {
	$hwid_hash = $_POST['hwid'];
	$username = $_POST['username'];
    $password = $_POST['password'];

	header("Content-Type: application/json");

	if (!isset($hwid_hash) || !isset($username) || !isset($password)) {
        echo json_encode([
			"status" => "CREDENTIALS_FAIL",
			"code" => 0x340
		]);
		return;
    }

    $users = $db->query(
		"SELECT id, password, banned, hwid FROM users WHERE username = ? LIMIT 1",
		[$username]
	);

    if (empty($users)) {
        echo json_encode([
			"status" => "USER_NOT_FOUND",
			"code" => 0x278
		]);
		return; 
    }

	$user = $users[0];

	if ($password !== $user['password']) {
        echo json_encode([
			"status" => "PASSWORD_FAIL",
			"code" => 0x114
		]);
        return;
    }

	if ((int)$user['banned'] === 1) {
        echo json_encode([
			"status" => "USER_BANNED",
			"code" => 0x98
		]);
        return;
    }

	if (empty($user['hwid'])) {
		$db->query(
			"UPDATE users SET hwid = ? WHERE id = ?",
			[$hwid_hash, $user['id']]
		);
	}
	elseif ($user['hwid'] !== $hwid_hash) {
		echo json_encode([
			"status" => "HWID_MISMATCH",
			"code" => 0x77
		]);
		return;
	}

	$user_id = $user['id'];
	$token = bin2hex(random_bytes(32));

    $subs = $db->query("SELECT game_name, expires_at FROM subs WHERE user_id = ?", [$user_id]);

	$response = [
		"status" => "OK",
		"code" => 0x2,
		"token" => $token,
		"games" => []
	];

	foreach ($subs as $row) {
		$game = $row['game_name'];
    
		$response["games"][$game] = [
			"expires_at" => $row['expires_at'] ?? ""
		];
	}

    $session_expires_at = date('Y-m-d H:i:s', strtotime('+3 minutes'));

    $db->query(
		"UPDATE users SET token = ?, token_expires_at = ? WHERE id = ?", 
		[$token, $session_expires_at, $user_id]
    );

	echo json_encode($response, JSON_UNESCAPED_SLASHES);
});

$route->get('/api/ban', function() use ($db) {
	$token = $_GET['token'];

	if (!isset($token)) {
		http_response_code(404);
		return;
	}

	header("Content-Type: application/json");

    $users = $db->query("SELECT id, token_expires_at FROM users WHERE token = ? LIMIT 1", [$token]);

    if (empty($users)) {
		echo json_encode([
			"status" => "TOKEN_NOT_FOUND",
			"code" => 0x67
		]);
		return;
	}

    $user = $users[0];

    if (strtotime($user['token_expires_at']) < time()) {
		echo json_encode([
			"status" => "TOKEN_EXPIRED",
			"code" => 0x69
		]);

        $db->query("UPDATE users SET token = NULL, token_expires_at = NULL WHERE id = ?", [$user['id']]);
        return;
    }

    $db->query(
        "UPDATE users SET banned = 1, token = NULL, token_expires_at = NULL WHERE id = ?", 
        [$user['id']]
    );
	
	echo json_encode([
		"status" => "OK",
		"code" => 0x70
	]);
});

$route->get('/api/dll', function() use ($db) {
	$game = $_GET['game'];
    $token = $_GET['token'];

	if (!isset($game) || !isset($token)) {
        http_response_code(404);
        return;
    }

	if (!is_numeric($game)) {
        http_response_code(404);
        return;
	}

	header("Content-Type: application/json");

    $users = $db->query("SELECT id, token_expires_at FROM users WHERE token = ? LIMIT 1", [$token]);

    if (empty($users)) {
		echo json_encode([
			"status" => "TOKEN_NOT_FOUND",
			"code" => 0x67
		]);
        return;
    }

    $user = $users[0];

    if (strtotime($user['token_expires_at']) < time()) {
		echo json_encode([
			"status" => "TOKEN_EXPIRED",
			"code" => 0x69
		]);

        $db->query("UPDATE users SET token = NULL, token_expires_at = NULL WHERE id = ?", [$user['id']]);
        return;
    }

	$subs = $db->query(
		"SELECT expires_at, dll_path FROM subs WHERE user_id = ? AND game_id = ? LIMIT 1",
		[$user['id'], $game]
	);

	if (empty($subs)) {
		echo json_encode([
			"status" => "SUB_EXPIRED",
			"code" => 0x76
		]);
		return;
	}

	$sub = $subs[0];

	if (strtotime($sub['expires_at']) < time()) {
		echo json_encode([
			"status" => "SUB_EXPIRED",
			"code" => 0x76
		]);
		return;
	}

	$payloadData = file_get_contents($sub['dll_path']);

	echo json_encode([
		"status" => "OK",
		"code" => 0x70,
		"base64-payload" => base64_encode($payloadData)
	], JSON_UNESCAPED_SLASHES);
});

$route->handle();

?>