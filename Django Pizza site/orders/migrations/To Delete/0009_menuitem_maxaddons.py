# Generated by Django 2.2.7 on 2019-11-18 01:51

import django.core.validators
from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('orders', '0008_auto_20191113_2041'),
    ]

    operations = [
        migrations.AddField(
            model_name='menuitem',
            name='maxAddOns',
            field=models.IntegerField(default=5, validators=[django.core.validators.MaxValueValidator(10), django.core.validators.MinValueValidator(0)]),
            preserve_default=False,
        ),
    ]
