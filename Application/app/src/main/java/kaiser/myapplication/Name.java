package kaiser.myapplication;

/**
 * Created by tjdqo_000 on 2018-02-19.
 */

public class Name {
    public double temperature = 0.0;
    public double huminity = 0.0;

    Name(){

    }

    public void setTemperature(double temperature) {
        this.temperature = temperature;
    }

    public void setHuminity(double huminity) {
        this.huminity = huminity;
    }

    public double getTemperature() {
        return temperature;
    }

    public double getHuminity() {
        return huminity;
    }
}
