package kaiser.myapplication;

import android.content.Context;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.google.firebase.database.ChildEventListener;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

import java.util.HashMap;

public class MainActivity extends AppCompatActivity {
    FirebaseDatabase firebaseDatabase;
    DatabaseReference databaseReference;
    Context mContext;
    EditText et_temperature;
    EditText et_humidity;
    Button btn_load;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        firebaseDatabase = FirebaseDatabase.getInstance();
        databaseReference = firebaseDatabase.getReference();
        mContext = this;
        et_temperature = findViewById(R.id.et_temperature);
        et_humidity = findViewById(R.id.et_huminity);
        btn_load = findViewById(R.id.btn_load);
        databaseReference.child("users").child("name").addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(DataSnapshot dataSnapshot) {
                HashMap<String, Double> name = (HashMap<String, Double>) dataSnapshot.getValue();
                et_temperature.setText(String.valueOf(name.get("temperature"))+"°F");
                et_humidity.setText(String.valueOf(name.get("humidity"))+"%");
                Toast.makeText(mContext, String.valueOf(name.get("temperature"))+"\n"+String.valueOf(name.get("huminity")), Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onCancelled(DatabaseError databaseError) {

            }
        });
        btn_load.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                databaseReference.child("users").child("name").addListenerForSingleValueEvent(new ValueEventListener() {
                    @Override
                    public void onDataChange(DataSnapshot dataSnapshot) {
                        HashMap<String, Double> name = (HashMap<String, Double>) dataSnapshot.getValue();
                        et_temperature.setText(String.valueOf(name.get("temperature"))+"°F");
                        et_humidity.setText(String.valueOf(name.get("humidity"))+"%");
                        Toast.makeText(mContext, String.valueOf(name.get("temperature"))+"\n"+String.valueOf(name.get("humidity")), Toast.LENGTH_SHORT).show();
                    }

                    @Override
                    public void onCancelled(DatabaseError databaseError) {

                    }
                });
            }
        });

    }
}
