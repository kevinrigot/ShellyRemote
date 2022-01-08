class Status {
    private:
        boolean on;
        int brightness;
    public:
        Status(boolean on, int brightness):
            on(on),brightness(brightness)
            {}
        boolean isOn(){return on;}
        int getBrightness(){return brightness;}
};