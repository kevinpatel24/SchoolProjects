# Generated by Django 2.2.7 on 2019-11-13 19:04

import django.core.validators
from django.db import migrations, models
import django.db.models.deletion


class Migration(migrations.Migration):

    dependencies = [
        ('orders', '0004_orderitem_maxaddons'),
    ]

    operations = [
        migrations.CreateModel(
            name='AddOnCount',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('name', models.CharField(max_length=20, unique=True)),
                ('numberOfAddOns', models.IntegerField(validators=[django.core.validators.MaxValueValidator(3), django.core.validators.MinValueValidator(0)])),
                ('price', models.DecimalField(decimal_places=2, max_digits=4)),
            ],
        ),
        migrations.AddField(
            model_name='orderitem',
            name='addOnCount',
            field=models.ForeignKey(default=0, on_delete=django.db.models.deletion.CASCADE, to='orders.AddOnCount'),
            preserve_default=False,
        ),
    ]