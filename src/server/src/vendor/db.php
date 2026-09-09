<?php

class DBManager {
    private PDO $pdo;

    public function __construct($host, $db, $user, $pass, $charset) {
        $dsn = "mysql:host=$host;dbname=$db;charset=$charset";
        
        $options = [
            PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
            PDO::ATTR_EMULATE_PREPARES   => false,
        ];

        try {
            $this->pdo = new PDO($dsn, $user, $pass, $options);
            $this->init_tables();
            $this->seed_data();
            
        } catch (PDOException $e) {
            die("DBManager: " . $e->getMessage());
        }
    }

    private function init_tables(): void {
        $this->pdo->exec("
            CREATE TABLE IF NOT EXISTS `users` (
                `id` INT(11) NOT NULL AUTO_INCREMENT,
                `username` VARCHAR(191) NOT NULL,
                `password` MEDIUMTEXT NOT NULL,
                `banned` INT(11) DEFAULT NULL,
                `hwid` VARCHAR(64) DEFAULT NULL,
                `token` VARCHAR(64) DEFAULT NULL,
                `token_expires_at` DATETIME DEFAULT NULL,
                PRIMARY KEY (`id`),
                UNIQUE KEY `idx_username` (`username`),
                KEY `idx_token` (`token`)
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
        ");

        $this->pdo->exec("
            CREATE TABLE IF NOT EXISTS `subs` (
                `id` INT(11) NOT NULL AUTO_INCREMENT,
                `user_id` INT(11) NOT NULL,
                `game_id` INT(11) NOT NULL,
                `game_name` VARCHAR(64) NOT NULL,
                `dll_path` VARCHAR(160) NOT NULL,
                `expires_at` DATETIME DEFAULT NULL,
                PRIMARY KEY (`id`),
                KEY `idx_user_id` (`user_id`),
                KEY `idx_game_id` (`game_id`)
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
        ");
    }

    private function seed_data(): void {
        $this->pdo->exec("
            INSERT IGNORE INTO `users` (`id`, `username`, `password`, `banned`, `hwid`, `token`, `token_expires_at`) 
            VALUES (1, 'admin', 'admin', 0, NULL, NULL, NULL)
        ");

        $this->pdo->exec("
            INSERT IGNORE INTO `subs` (`id`, `user_id`, `game_id`, `game_name`, `dll_path`, `expires_at`) 
            VALUES 
            (1, 1, 0, 'Counter-Strike: Global Offensive', '../src/payloads/csgo.dll', '2027-04-06 22:57:29'),
            (2, 1, 1, 'Rust', '../src/payloads/rust.dll', '2027-02-10 22:57:47')
        ");
    }

    public function query(string $sql, array $params = []): array|int {
        $stmt = $this->pdo->prepare($sql);
        $stmt->execute($params);

        if (stripos($sql, 'select') === 0) return $stmt->fetchAll();

        return $stmt->rowCount();
    }
}

?>