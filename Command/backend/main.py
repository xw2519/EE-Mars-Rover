from uvicorn import run

if __name__ == "__main__":
    run("app.api:app", host="0.0.0.0", port=8000, reload=True)