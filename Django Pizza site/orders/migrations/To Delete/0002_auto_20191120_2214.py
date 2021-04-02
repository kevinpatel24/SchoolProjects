# Generated by Django 2.2.7 on 2019-11-20 22:14

from django.db import migrations, models
import django.db.models.deletion


class Migration(migrations.Migration):

    dependencies = [
        ('orders', '0001_initial'),
    ]

    operations = [
        migrations.AddField(
            model_name='addoncount',
            name='addOnType',
            field=models.ForeignKey(blank=True, default='Topping', on_delete=django.db.models.deletion.CASCADE, to='orders.AddOnType'),
        ),
        migrations.AddField(
            model_name='addoncount',
            name='category',
            field=models.ForeignKey(default='Regular Pizzas', on_delete=django.db.models.deletion.CASCADE, to='orders.Category'),
        ),
        migrations.AddField(
            model_name='addoncount',
            name='size',
            field=models.ForeignKey(blank=True, default='Small', on_delete=django.db.models.deletion.CASCADE, to='orders.Size'),
        ),
    ]
