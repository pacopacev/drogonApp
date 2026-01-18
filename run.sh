#!/bin/bash

cd /var/www/drogonApp

echo "Starting DrogonApp..."
echo "Using Aiven PostgreSQL: pa-pgdimitrov-bfdb.j.aivencloud.com:25464"

# Kill any existing instance
pkill -f DrogonApp 2>/dev/null
sleep 1

# Start the application
./DrogonApp 2>&1 | tee -a drogon_runtime.log &
APP_PID=$!

echo "Application started with PID: $APP_PID"

# Wait a bit and check if it's running
sleep 3

if ps -p $APP_PID > /dev/null; then
    echo "✅ DrogonApp is running (PID: $APP_PID)"
    echo "🌐 Access at: http://127.0.0.1:8080"
    echo "📝 Logs: tail -f drogon_runtime.log"
    
    # Test the endpoint
    echo ""
    echo "Testing endpoint..."
    curl -s -o /dev/null -w "HTTP Status: %{http_code}\n" http://127.0.0.1:8080/ || echo "Connection test failed"
else
    echo "❌ Failed to start DrogonApp"
    echo "Last 20 lines of log:"
    tail -20 drogon_runtime.log
fi
