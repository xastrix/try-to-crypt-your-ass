<?php

class RouteManager {
    private array $routes = [];

    public function get(string $path, callable $callback): void {
        $this->routes['GET'][$path] = $callback;
    }

    public function post(string $path, callable $callback): void {
        $this->routes['POST'][$path] = $callback;
    }

    public function handle(): void {
        $method = $_SERVER['REQUEST_METHOD'];
        $uri = parse_url($_SERVER['REQUEST_URI'], PHP_URL_PATH);

        if (!isset($this->routes[$method])) {
            $this->not_found();
            return;
        }

        foreach ($this->routes[$method] as $route => $callback) {
            $pattern = preg_replace('/\{[^\}]+\}/', '([^/]+)', $route);
            $pattern = '#^' . str_replace('/', '\/', $pattern) . '$#';

            if (preg_match($pattern, $uri, $matches)) {
                array_shift($matches);
                call_user_func_array($callback, $matches);
                return;
            }
        }

        $this->not_found();
    }

    private function not_found(): void {
        header("HTTP/1.0 404 Not Found");
    }
}

?>