# Generated by Django 2.2.7 on 2019-11-20 22:17

from django.db import migrations, models
import django.db.models.deletion


class Migration(migrations.Migration):

    dependencies = [
        ('orders', '0002_auto_20191120_2214'),
    ]

    operations = [
        migrations.AlterField(
            model_name='addoncount',
            name='addOnType',
            field=models.ForeignKey(blank=True, default='None', on_delete=django.db.models.deletion.CASCADE, to='orders.AddOnType'),
        ),
    ]
